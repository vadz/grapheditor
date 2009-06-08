/////////////////////////////////////////////////////////////////////////////
// Name:        graphctrl.cpp
// Purpose:     Graph control
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// Notes:
//
// One of the requirements of the graph control was that its interface should
// not depend on the underlying graphics library (OGL). Therefore, the graph
// control does not derive from a wxShapeCanvas and extend it to add a few
// features such as graph layout, but instead it owns a wxShapeCanvas as a
// child window. Similarly for the graph elements, they own wxShapes rather
// than deriving from them. The wxShape objects are connected to the
// corresponding GraphElement object using the wxShape's client data field.
//
// The graph elements provide two ways to customise their appearance in a way
// independent of the underlying graphics library. Firstly, there is the
// SetStyle method, which can select from a limited set of predefined shapes.
// Or for more control, there is an OnDraw overridable which can be used to
// draw the element manually.
//
// The abstracted API is of course more limited than that of the underlying
// graphics library, so access to the underlying graphics library is also
// available, though using it obviously makes makes the user code depend on
// the particular graphics library.
//
// The GraphCtrl has a method GetCanvas which returns the OGL canvas, and
// graph elements have methods SetShape/GetShape which can be used to assign
// a wxShape. The graph control customised the behaviour of the shapes added
// using wxShapeEvtHandler rather than by overriding wxShape methods,
// therefore pretty much any wxShape can be used, as long as they don't use
// the client data field for anything else.

#include "graphctrl.h"
#include <wx/tipwin.h>
#include <wx/ogl/ogl.h>
#include <bitset>
#include <set>

#ifndef NO_GRAPHVIZ
#include <gvc.h>
#include <gd.h>
#endif

#if defined(__VISUALC__) && !defined(NDEBUG)
    #define SUPPRESS_GRAPHVIZ_MEMLEAKS
    #include <crtdbg.h>
#endif

namespace tt_solutions {

using namespace std;
using namespace impl;

// ----------------------------------------------------------------------------
// Events
// ----------------------------------------------------------------------------

// Graph Events

DEFINE_EVENT_TYPE(Evt_Graph_Node_Add)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Delete)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Move)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Size)

DEFINE_EVENT_TYPE(Evt_Graph_Edge_Add)
DEFINE_EVENT_TYPE(Evt_Graph_Edge_Delete)

DEFINE_EVENT_TYPE(Evt_Graph_Connect_Feedback)
DEFINE_EVENT_TYPE(Evt_Graph_Connect)

// GraphCtrl Events

DEFINE_EVENT_TYPE(Evt_Graph_Node_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Activate)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Menu)

DEFINE_EVENT_TYPE(Evt_Graph_Edge_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Edge_Activate)
DEFINE_EVENT_TYPE(Evt_Graph_Edge_Menu)

DEFINE_EVENT_TYPE(Evt_Graph_Ctrl_Zoom)

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

namespace {

// sort order for the elements when loading
const wxString SORT_ELEMENT = _T("el");
const wxString SORT_NODE    = SORT_ELEMENT + _T("1");
const wxString SORT_EDGE    = SORT_ELEMENT + _T("2");

// the wxShape's client data field points back to the GraphElement
GraphElement *GetElement(wxShape *shape)
{
    void *data = shape->GetClientData();
    return data ? wxStaticCast(data, GraphElement) : NULL;
}

GraphEdge *GetEdge(wxLineShape *line)
{
    void *data = line->GetClientData();
    return data ? wxStaticCast(data, GraphEdge) : NULL;
}

GraphNode *GetNode(wxShape *shape)
{
    void *data = shape->GetClientData();
    return data ? wxStaticCast(data, GraphNode) : NULL;
}

// lexicographic order for positions, top to bottom, left to right.
bool operator<(const wxPoint& pt1, const wxPoint &pt2)
{
    return pt1.y < pt2.y || (pt1.y == pt2.y && pt1.x < pt2.x);
}

// a compare functor for elements that orders them by their screen positions
class ElementCompare
{
public:
    bool operator()(const GraphElement *n, const GraphElement *m) const
    {
        return n->GetPosition() < m->GetPosition();
    }
};

wxString NodeName(const GraphNode& node)
{
    return wxString::Format(_T("n%p"), &node);
}

wxShapeCanvas *GetCanvas(wxShape *shape)
{
    return shape ? shape->GetCanvas() : NULL;
}

char *unconst(const char *str)
{
    return const_cast<char*>(str);
}

wxPolygonShape *CreatePolygon(int num_points, const int points[][2])
{
    wxList *list = new wxList;

    for (int i = 0; i < num_points; i++)
        list->Append(reinterpret_cast<wxObject*>
                     (new wxRealPoint(points[i][0], points[i][1])));

    wxPolygonShape *shape = new wxPolygonShape;
    shape->Create(list);
    return shape;
}

int between(int value, int limit1, int limit2)
{
    int lo = min(limit1, limit2);
    int hi = max(limit1, limit2);
    value = max(value, lo);
    value = min(value, hi);
    return value;
}

bool ShowLine(wxLineShape *line, GraphNode *from, GraphNode *to)
{
    if (!line || !from || !to)
        return false;

    wxShape *fromshape = from->GetShape();
    wxShape *toshape = to->GetShape();

    if (!fromshape || !toshape)
        return false;

    fromshape->AddLine(line, toshape);

    double x1, y1, x2, y2;
    line->FindLineEndPoints(&x1, &y1, &x2, &y2);
    line->SetEnds(x1, y1, x2, y2);

    return true;
}

wxFont DefaultFont()
{
    static wxFont font(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Arial"));
    return font;
}

wxSize GetScreenDPI()
{
    static wxSize dpi(wxScreenDC().GetPPI());
    return dpi;
}

} // namespace

// ----------------------------------------------------------------------------
// Initialisor
// ----------------------------------------------------------------------------

namespace impl {

int Initialisor::m_initalise;

Initialisor::Initialisor()
{
    if (m_initalise++ == 0)
        wxOGLInitialize();
}

Initialisor::~Initialisor()
{
    wxASSERT(m_initalise > 0);
    if (--m_initalise == 0)
        wxOGLCleanUp();
}

} // namespace impl

// ----------------------------------------------------------------------------
// GraphEvent
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphEvent, wxNotifyEvent)

GraphEvent::GraphEvent(wxEventType commandType, int winid)
  : wxNotifyEvent(commandType, winid),
    m_node(NULL),
    m_target(NULL),
    m_edge(NULL),
    m_sources(NULL),
    m_zoom(0)
{
}

GraphEvent::GraphEvent(const GraphEvent& event)
  : wxNotifyEvent(event),
    m_pos(event.m_pos),
    m_node(event.m_node),
    m_target(event.m_target),
    m_edge(event.m_edge),
    m_sources(event.m_sources),
    m_zoom(event.m_zoom)
{
}

// ----------------------------------------------------------------------------
// Canvas
// ----------------------------------------------------------------------------

namespace impl {

class GraphCanvas : public wxShapeCanvas
{
public:
    GraphCanvas(wxWindow *parent = NULL, wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxBORDER | wxRETAINED,
                 const wxString& name = DefaultName);
    ~GraphCanvas();

    static const wxChar DefaultName[];

    void OnLeftClick(double x, double y, int keys);

    void OnDragLeft(bool draw, double x, double y, int keys);
    void OnBeginDragLeft(double x, double y, int keys);
    void OnEndDragLeft(double x, double y, int keys);

    void SetGraph(Graph *graph) { m_graph = graph; }
    Graph *GetGraph() const { return m_graph; }

    bool ProcessEvent(wxEvent& event);

    void OnScroll(wxScrollWinEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnSetFocus(wxFocusEvent& event);
    void OnLeftButton(wxMouseEvent& event);
    void OnRightButton(wxMouseEvent& event);

    void PrepareDC(wxDC& dc);

    void AdjustScrollbars() { }

    void SetCheckBounds() { m_checkBounds = true; }
    bool GetCheckBounds() { return m_checkBounds; }
    bool CheckBounds();

    void DoScroll(int orient, int type, int pos = 0, int lines = 1);

    void ScrollTo(const wxPoint& ptGraph, bool draw = true);
    void ScrollTo(int side, bool draw = true);
    wxPoint ScrollByOffset(int x, int y, bool draw = true);
    void EnsureVisible(wxRect rc, bool draw = true);

    wxPoint GetScroll() const;
    wxPoint GetScrollPosition();

    void SetBorder(const wxSize& size) { m_border = size; SetCheckBounds(); }
    wxSize GetBorder() const { return m_border; }

    void SetBorderType(int type) { m_borderType = type; SetCheckBounds(); }
    int GetBorderType() const { return m_borderType; }

    void SetMargin(const wxSize& size) { m_margin = size; SetCheckBounds(); }
    wxSize GetMargin() const { return m_margin; }

    wxRect ScreenToGraph(const wxRect& rcScreen);
    wxRect GraphToScreen(const wxRect& rcGraph);

    inline wxSize GetScrollClientSize() const;
    inline wxSize GetFullClientSize() const;

    void SetFits() { m_fitsX = m_fitsY = true; }

private:
    static wxWindow *EnsureParent(wxWindow *parent);

    Graph *m_graph;
    bool m_isPanning;
    bool m_checkBounds;
    wxPoint m_ptDrag;
    wxPoint m_ptOrigin;
    wxSize m_sizeScrollbar;
    wxSize m_border;
    int m_borderType;
    wxSize m_margin;
    bool m_fitsX;
    bool m_fitsY;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(GraphCanvas)
    DECLARE_NO_COPY_CLASS(GraphCanvas)
};

const wxChar GraphCanvas::DefaultName[] = _T("graph_canvas");

IMPLEMENT_DYNAMIC_CLASS(GraphCanvas, wxShapeCanvas)

BEGIN_EVENT_TABLE(GraphCanvas, wxShapeCanvas)
    EVT_SCROLLWIN(GraphCanvas::OnScroll)
    EVT_IDLE(GraphCanvas::OnIdle)
    EVT_PAINT(GraphCanvas::OnPaint)
    EVT_SIZE(GraphCanvas::OnSize)
    EVT_LEFT_DOWN(GraphCanvas::OnLeftButton)
    EVT_LEFT_UP(GraphCanvas::OnLeftButton)
    EVT_RIGHT_DOWN(GraphCanvas::OnRightButton)
    EVT_RIGHT_UP(GraphCanvas::OnRightButton)
    EVT_SET_FOCUS(GraphCanvas::OnSetFocus)
END_EVENT_TABLE()

GraphCanvas::GraphCanvas(
        wxWindow *parent,
        wxWindowID id,
        const wxPoint& pos,
        const wxSize& size,
        long style,
        const wxString& name)
  : wxShapeCanvas(EnsureParent(parent), id, pos, size, style, name),
    m_graph(NULL),
    m_isPanning(false),
    m_checkBounds(false),
    m_border(0, 0),
    m_borderType(GraphCtrl::Percentage_Border),
    m_margin(GetScreenDPI() / 4),
    m_fitsX(true),
    m_fitsY(true)
{
    SetScrollRate(1, 1);
    SetFont(DefaultFont());
}

GraphCanvas::~GraphCanvas()
{
    wxFrame *dummy = wxDynamicCast(GetParent(), wxFrame);
    if (dummy)
        dummy->Destroy();
}

bool GraphCanvas::ProcessEvent(wxEvent& event)
{
    if (event.IsKindOf(CLASSINFO(wxMouseEvent)))
        if (GetParent()->GetEventHandler()->ProcessEvent(event))
            return true;

    return wxShapeCanvas::ProcessEvent(event);
}

void GraphCanvas::OnSetFocus(wxFocusEvent&)
{
    GetParent()->SetFocus();
}

void GraphCanvas::OnLeftButton(wxMouseEvent& event)
{
    if (m_dragState == StartDraggingRight ||
        m_dragState == ContinueDraggingRight)
        return;

    if (event.LeftDown()) {
        if (FindFocus() != GetParent())
            SetFocus();

        if (event.ShiftDown() && !event.Dragging()) {
            wxClientDC dc(this);
            PrepareDC(dc);
            wxPoint pt(event.GetLogicalPosition(dc));
            m_checkTolerance = true;
            m_draggedShape = NULL;
            m_dragState = StartDraggingLeft;
            m_firstDragX = double(pt.x);
            m_firstDragY = double(pt.y);
            return;
        }
    }

    event.Skip();
}

void GraphCanvas::OnRightButton(wxMouseEvent& event)
{
    if (m_dragState == StartDraggingLeft ||
        m_dragState == ContinueDraggingLeft)
        return;

    event.Skip();
}

wxWindow *GraphCanvas::EnsureParent(wxWindow *parent)
{
    if (!parent) {
        parent = new wxFrame(NULL, wxID_ANY, DefaultName,
                             wxDefaultPosition, wxDefaultSize, 0);
        wxTopLevelWindows.remove(parent);
    }
    return parent;
}

void GraphCanvas::OnLeftClick(double, double, int)
{
    GetGraph()->UnselectAll();
}

void GraphCanvas::OnBeginDragLeft(double x, double y, int keys)
{
    if ((keys & KEY_SHIFT) != 0) {
        // panning
        m_isPanning = true;
        m_ptDrag = wxGetMousePosition();
    }
    else {
        // rubber banding
        m_ptDrag = wxPoint(int(x), int(y));
    }
    CaptureMouse();
}

void GraphCanvas::OnDragLeft(bool, double x, double y, int)
{
    if (m_isPanning) {
        // The mouse event that lead here may have been in queue before the
        // last time the origin changed, in which case the x and y parameters
        // would need adjusting to allow for that. Or OTOH the event may have
        // come after and the x, y parameters need no adjustment. A simple
        // way to avoid this problem is to use the global mouse position
        // instead.
        wxMouseState mouse = wxGetMouseState();

        if (mouse.LeftDown()) {
            m_ptDrag -= ScrollByOffset(m_ptDrag.x - mouse.GetX(),
                                       m_ptDrag.y - mouse.GetY());
            Update();
        }
    }
    else {
        // rubber banding
        wxClientDC dc(this);
        PrepareDC(dc);

        wxPen dottedPen(*wxBLACK, 1, wxDOT);
        dc.SetLogicalFunction(OGLRBLF);
        dc.SetPen(dottedPen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);

        wxSize size(int(x) - m_ptDrag.x, int(y) - m_ptDrag.y);
        dc.DrawRectangle(m_ptDrag, size);
    }
}

void GraphCanvas::OnEndDragLeft(double x, double y, int key)
{
    ReleaseMouse();

    if (m_isPanning) {
        // finish panning
        m_isPanning = false;
        m_checkBounds = true;
        // these aren't needed I think
        SetScrollPos(wxHORIZONTAL, GetScrollPos(wxHORIZONTAL));
        SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL));
    }
    else {
        // rubber banding
        wxRect rc;

        if (x >= m_ptDrag.x) {
            rc.x = m_ptDrag.x;
            rc.width = int(x) - m_ptDrag.x;
        } else {
            rc.x = int(x);
            rc.width = m_ptDrag.x - int(x);
        }

        if (y >= m_ptDrag.y) {
            rc.y = m_ptDrag.y;
            rc.height = int(y) - m_ptDrag.y;
        } else {
            rc.y = int(y);
            rc.height = m_ptDrag.y - int(y);
        }

        Graph *graph = GetGraph();
        Graph::iterator i, j, end;
        tie(i, end) = graph->GetElements();

        while (i != end)
        {
            // Increment i before calling Select, since Select invalidates
            // iterators to that element.
            j = i++;

            if (!j->IsSelected()) {
                if (rc.Intersects(j->GetBounds().Inflate(1)))
                    j->Select();
            }
            else {
                if ((key & KEY_CTRL) == 0 && !rc.Intersects(j->GetBounds()))
                    j->Unselect();
            }
        }
    }
}

void GraphCanvas::DoScroll(int orient, int type, int pos, int lines)
{
    bool horz = orient == wxHORIZONTAL;
    int scroll = horz ? m_xScrollPosition : m_yScrollPosition;
    int size = horz ? m_virtualSize.x : m_virtualSize.y;
    int cs = horz ? GetClientSize().x : GetClientSize().y;

    if (type == wxEVT_SCROLLWIN_TOP)
        pos = 0;
    else if (type == wxEVT_SCROLLWIN_BOTTOM)
        pos = size;
    else if (type == wxEVT_SCROLLWIN_LINEUP)
        pos = scroll - 16 * lines;
    else if (type == wxEVT_SCROLLWIN_LINEDOWN)
        pos = scroll + 16 * lines;
    else if (type == wxEVT_SCROLLWIN_PAGEUP)
        pos = scroll - cs;
    else if (type == wxEVT_SCROLLWIN_PAGEDOWN)
        pos = scroll + cs;

    if (pos > size - cs)
        pos = size - cs;
    if (pos < 0)
        pos = 0;

    if (horz)
        ScrollByOffset(pos - m_xScrollPosition, 0);
    else
        ScrollByOffset(0, pos - m_yScrollPosition);
}

void GraphCanvas::OnScroll(wxScrollWinEvent& event)
{
    if (FindFocus() != GetParent())
        SetFocus();

    int orient = event.GetOrientation();
    int type = event.GetEventType();
    int pos = event.GetPosition();

    DoScroll(orient, type, pos);
}

// The scrollbars are sized to allow scrolling over the bounding rectangle of
// all the nodes currently in the graph. Panning allows scrolling beyond this
// region, and in this case the scrollbars adjust to include the current
// position.
//
// Code that moves nodes or otherwise affects the scrollbars, sets the flag
// SetCheckBounds(). Then in the idle time the bounding rectangle of all the
// nodes is recalculated and the scrollbars adjusted if necessary.
//
// The reason this is done during the idle time, is that scrolling back to
// the origin may cause the scrollbars to disappear, and when this is done
// using the mouse, the mouse then stops working. Presumably because the
// scrollbar is destroyed without releasing the capture.
//
void GraphCanvas::OnIdle(wxIdleEvent&)
{
    wxMouseState state = wxGetMouseState();

    if (m_checkBounds && !state.LeftDown())
        CheckBounds();

    if (state.ShiftDown())
        SetCursor(wxCURSOR_SIZING);
    else
        SetCursor(wxCURSOR_DEFAULT);
}

bool GraphCanvas::CheckBounds()
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxSize cs = GetFullClientSize();
    wxSize fullclient = cs;

    wxRect b = m_graph->GetBounds();

    if (!b.IsEmpty()) {
        if (m_borderType == GraphCtrl::Graph_Border)
            b.Inflate(m_border);

        b.x = dc.LogicalToDeviceX(b.x);
        b.y = dc.LogicalToDeviceY(b.y);
        b.width = dc.LogicalToDeviceXRel(b.width);
        b.height = dc.LogicalToDeviceYRel(b.height);

        if (m_borderType == GraphCtrl::Percentage_Border)
            b.Inflate(wxSize(cs.x * m_border.x, cs.y * m_border.y) / 100);
        else if (m_borderType == GraphCtrl::Ctrl_Border)
            b.Inflate(m_border);

        wxRect inner = m_graph->GetBounds();
        inner.Inflate(m_margin);
        inner.x = dc.LogicalToDeviceX(inner.x);
        inner.y = dc.LogicalToDeviceY(inner.y);
        inner.width = dc.LogicalToDeviceXRel(inner.width);
        inner.height = dc.LogicalToDeviceYRel(inner.height);
        b.Union(inner);
    }

    const wxRect b0 = b;

    int x = -min(0, b.x);
    int y = -min(0, b.y);

    bool fitsX = m_fitsX, fitsY = m_fitsY;

    m_fitsX = b.width <= cs.x;
    m_fitsY = b.height <= cs.y;

    bool needHBar = false, needVBar = false;
    bool done = false;

    while (!done && !b.IsEmpty()) {
        done = true;

        b = b0;
        wxRect csr = m_fitsY ? wxRect(fullclient) : wxRect(cs);
        b.Union(csr.CentreIn(b));
        m_fitsX = b.width <= csr.width;

        x = -min(0, b.x);
        m_virtualSize.x = max(cs.x, b.GetRight()) + x;

        if (m_virtualSize.x > cs.x) {
            SetScrollbar(wxHORIZONTAL, x, cs.x, m_virtualSize.x);
            GetClientSize(NULL, &cs.y);
            needHBar = true;
        }
        else {
            cs.y = fullclient.y;
            needHBar = false;
        }

        b = b0;
        csr = m_fitsX ? wxRect(fullclient) : wxRect(cs);
        b.Union(csr.CentreIn(b));
        m_fitsY = b.height <= csr.height;

        y = -min(0, b.y);
        m_virtualSize.y = max(cs.y, b.GetBottom()) + y;

        if (m_virtualSize.y > cs.y) {
            SetScrollbar(wxVERTICAL, y, cs.y, m_virtualSize.y);
            GetClientSize(&cs.x, NULL);
            done = needVBar;
            needVBar = true;
        }
        else {
            cs.x = fullclient.x;
            needVBar = false;
        }
    }

    m_ptOrigin.x += x - m_xScrollPosition;
    m_xScrollPosition = x;

    m_ptOrigin.y += y - m_yScrollPosition;
    m_yScrollPosition = y;

    if (!needHBar)
        SetScrollbar(wxHORIZONTAL, 0, 0, 0);
    if (!needVBar)
        SetScrollbar(wxVERTICAL, 0, 0, 0);

    m_sizeScrollbar = fullclient - cs;

    m_checkBounds = false;

    return m_fitsX != fitsX || m_fitsY != fitsY;
}

wxSize GraphCanvas::GetScrollClientSize() const
{
    wxSize cs = GetClientSize();
    if (m_fitsY)
        cs.x += m_sizeScrollbar.x;
    if (m_fitsX)
        cs.y += m_sizeScrollbar.y;
    return cs;
}

wxSize GraphCanvas::GetFullClientSize() const
{
    return GetClientSize() + m_sizeScrollbar;
}

void GraphCanvas::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    PrepareDC(dc);

    if (GetDiagram())
        GetDiagram()->Redraw(dc);
}

void GraphCanvas::OnSize(wxSizeEvent& event)
{
    SetCheckBounds();
    event.Skip();
}

void GraphCanvas::PrepareDC(wxDC& dc)
{
    int x = m_ptOrigin.x - m_xScrollPosition;
    int y = m_ptOrigin.y - m_yScrollPosition;

    dc.SetDeviceOrigin(x, y);
    dc.SetUserScale(m_scaleX, m_scaleY);
}

wxPoint GraphCanvas::GetScroll() const
{
    return wxPoint(m_xScrollPosition, m_yScrollPosition);
}

void GraphCanvas::ScrollTo(const wxPoint& ptGraph, bool draw)
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxSize cs = GetScrollClientSize();
    int x = dc.LogicalToDeviceX(ptGraph.x) - cs.x / 2;
    int y = dc.LogicalToDeviceY(ptGraph.y) - cs.y / 2;

    ScrollByOffset(x, y, draw);
}

wxPoint GraphCanvas::GetScrollPosition()
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxSize cs = GetScrollClientSize();
    wxPoint pt;

    pt.x = dc.DeviceToLogicalX(cs.x / 2);
    pt.y = dc.DeviceToLogicalX(cs.y / 2);

    return pt;
}

wxPoint GraphCanvas::ScrollByOffset(int x, int y, bool draw)
{
    m_xScrollPosition += x;
    m_yScrollPosition += y;

    if (draw)
        ScrollWindow(-x, -y);

    SetCheckBounds();

    return wxPoint(x, y);
}

void GraphCanvas::EnsureVisible(wxRect rcGraph, bool draw)
{
    wxClientDC dc(this);
    PrepareDC(dc);

    rcGraph.Inflate(m_margin);

    wxRect rc(dc.LogicalToDeviceX(rcGraph.x),
              dc.LogicalToDeviceY(rcGraph.y),
              dc.LogicalToDeviceXRel(rcGraph.width),
              dc.LogicalToDeviceYRel(rcGraph.height));

    wxRect rcClient = GetScrollClientSize();

    int x = 0, y = 0;

    if (rc.x < rcClient.x)
        x = rc.x - rcClient.x;
    else if (rc.GetRight() > rcClient.GetRight())
        x = rc.GetRight() - rcClient.GetRight();

    if (rc.y < rcClient.y)
        y = rc.y - rcClient.y;
    else if (rc.GetBottom() > rcClient.GetBottom())
        y = rc.GetBottom() - rcClient.GetBottom();

    if (x || y)
        ScrollByOffset(x, y, draw);
}

void GraphCanvas::ScrollTo(int side, bool draw)
{
    wxASSERT(m_graph);

    wxClientDC dc(this);
    PrepareDC(dc);

    wxRect rcGraph = m_graph->GetBounds();
    rcGraph.Inflate(m_margin);

    wxRect rc(dc.LogicalToDeviceX(rcGraph.x),
              dc.LogicalToDeviceY(rcGraph.y),
              dc.LogicalToDeviceXRel(rcGraph.width),
              dc.LogicalToDeviceYRel(rcGraph.height));

    wxRect rcClient = GetScrollClientSize();

    rc += rcClient.CentreIn(rc);

    int x = 0, y = 0;

    if ((side & wxLEFT) != 0)
        x = rc.x - rcClient.x;
    else if ((side & wxRIGHT) != 0)
        x = rc.GetRight() - rcClient.GetRight();
    else if (rc.x > rcClient.x)
        x = rc.x - rcClient.x;
    else if (rc.GetRight() < rcClient.GetRight())
        x = rc.GetRight() - rcClient.GetRight();

    if ((side & wxTOP) != 0)
        y = rc.y - rcClient.y;
    else if ((side & wxBOTTOM) != 0)
        y = rc.GetBottom() - rcClient.GetBottom();
    else if (rc.y > rcClient.y)
        y = rc.y - rcClient.y;
    else if (rc.GetBottom() < rcClient.GetBottom())
        y = rc.GetBottom() - rcClient.GetBottom();

    if (x || y)
        ScrollByOffset(x, y, draw);
}

wxRect GraphCanvas::ScreenToGraph(const wxRect& rcScreen)
{
    wxClientDC dc(this);
    PrepareDC(dc);
    wxPoint pt = ScreenToClient(rcScreen.GetTopLeft());
    return wxRect(dc.DeviceToLogicalX(pt.x),
                  dc.DeviceToLogicalY(pt.y),
                  dc.DeviceToLogicalXRel(rcScreen.width),
                  dc.DeviceToLogicalYRel(rcScreen.height));
}

wxRect GraphCanvas::GraphToScreen(const wxRect& rcGraph)
{
    wxClientDC dc(this);
    PrepareDC(dc);
    wxPoint pt( dc.LogicalToDeviceX(rcGraph.x),
                dc.LogicalToDeviceY(rcGraph.y));
    wxSize size(dc.LogicalToDeviceXRel(rcGraph.width),
                dc.LogicalToDeviceYRel(rcGraph.height));
    return wxRect(ClientToScreen(pt), size);
}

} // namespace impl

// ----------------------------------------------------------------------------
// Handler to give shapes a transparent background
// ----------------------------------------------------------------------------

namespace {

class GraphHandler: public wxShapeEvtHandler
{
public:
    GraphHandler(wxShapeEvtHandler *prev);

    void OnErase(wxDC& dc);
    void OnMoveLink(wxDC& dc, bool moveControlPoints);

protected:
    virtual wxRect GetEraseRect() const;
};

GraphHandler::GraphHandler(wxShapeEvtHandler *prev)
  : wxShapeEvtHandler(prev, prev->GetShape())
{
}

void GraphHandler::OnErase(wxDC& dc)
{
    wxShape *shape = GetShape();

    if (GetShape()->IsShown() && dc.IsKindOf(CLASSINFO(wxWindowDC)))
    {
        wxRect rc = GetEraseRect();

        rc.x = dc.LogicalToDeviceX(rc.x);
        rc.y = dc.LogicalToDeviceY(rc.y);
        rc.width = dc.LogicalToDeviceXRel(rc.width);
        rc.height = dc.LogicalToDeviceYRel(rc.height);

        wxWindow *canvas = shape->GetCanvas();
        canvas->RefreshRect(rc);
    }
}

wxRect GraphHandler::GetEraseRect() const
{
    wxShape *shape = GetShape();
    wxPen *pen = GetShape()->GetPen();
    int penWidth = pen ? pen->GetWidth() : 0;

    double sizeX, sizeY;
    shape->GetBoundingBoxMax(&sizeX, &sizeY);
    sizeX += 2 * penWidth;
    sizeY += 2 * penWidth;

    int x1 = int(floor(shape->GetX() - sizeX / 2));
    int y1 = int(floor(shape->GetY() - sizeY / 2));
    int x2 = int(ceil(shape->GetX() + sizeX / 2));
    int y2 = int(ceil(shape->GetY() + sizeY / 2));

    return wxRect(x1, y1, x2 - x1, y2 - y1);
}

void GraphHandler::OnMoveLink(wxDC& dc, bool moveControlPoints)
{
    wxShape *shape = GetShape();
    shape->Show(false);
    wxShapeEvtHandler::OnMoveLink(dc, moveControlPoints);
    shape->Show(true);
}

// ----------------------------------------------------------------------------
// Handler for control points
// ----------------------------------------------------------------------------

class ControlPointHandler: public GraphHandler
{
public:
    ControlPointHandler(wxShapeEvtHandler *prev);

    void OnErase(wxDC& dc);
};

ControlPointHandler::ControlPointHandler(wxShapeEvtHandler *prev)
  : GraphHandler(prev)
{
}

void ControlPointHandler::OnErase(wxDC& dc)
{
    wxControlPoint *control = wxStaticCast(GetShape(), wxControlPoint);
    control->SetX(control->m_shape->GetX() + control->m_xoffset);
    control->SetY(control->m_shape->GetY() + control->m_yoffset);
    GraphHandler::OnErase(dc);
}

// ----------------------------------------------------------------------------
// Handler allow selection
// ----------------------------------------------------------------------------

class GraphElementHandler: public GraphHandler
{
public:
    GraphElementHandler(wxShapeEvtHandler *prev);

    void OnLeftClick(double x, double y, int keys, int attachment);
    void OnLeftDoubleClick(double x, double y, int keys, int attachment);
    void OnRightClick(double x, double y, int keys, int attachment);

    void OnDraw(wxDC& dc)       { GetElement(GetShape())->OnDraw(dc); }
    void OnDrawContents(wxDC&)  { }

protected:
    void HandleClick(wxEventType cmd, double x, double y, int keys);
    void HandleDClick(wxEventType cmd, double x, double y, int keys);
    void HandleRClick(wxEventType cmd, double x, double y, int keys);

    bool SendEvent(wxEventType cmd, double x, double y);

    void Select(wxShape *shape, bool select, int keys);
};

GraphElementHandler::GraphElementHandler(wxShapeEvtHandler *prev)
  : GraphHandler(prev)
{
}

void GraphElementHandler::OnLeftClick(double x, double y, int keys, int)
{
    HandleClick(Evt_Graph_Edge_Click, x, y, keys);
}

void GraphElementHandler::OnLeftDoubleClick(double x, double y,
                                            int keys, int)
{
    HandleDClick(Evt_Graph_Edge_Activate, x, y, keys);
}

void GraphElementHandler::OnRightClick(double x, double y, int keys, int)
{
    HandleRClick(Evt_Graph_Edge_Menu, x, y, keys);
}

void GraphElementHandler::HandleClick(wxEventType cmd,
                                      double x, double y,
                                      int keys)
{
    wxShape *shape = GetShape();
    Select(shape, !shape->Selected(), keys);
    SendEvent(cmd, x, y);
}

void GraphElementHandler::HandleDClick(wxEventType cmd,
                                       double x, double y,
                                       int keys)
{
    wxShape *shape = GetShape();
    if (!shape->Selected())
        Select(shape, true, keys);
    SendEvent(cmd, x, y);
}

void GraphElementHandler::HandleRClick(wxEventType cmd,
                                       double x, double y,
                                       int keys)
{
    wxShape *shape = GetShape();
    if (!shape->Selected())
        Select(shape, true, keys);
    SendEvent(cmd, x, y);
}

bool GraphElementHandler::SendEvent(wxEventType cmd, double x, double y)
{
    wxShape *shape = GetShape();
    wxShapeCanvas *canvas = shape->GetCanvas();

    GraphElement *element = GetElement(shape);
    GraphNode *node = wxDynamicCast(element, GraphNode);
    GraphEdge *edge = node ? NULL : wxDynamicCast(element, GraphEdge);

    GraphEvent event(cmd, canvas->GetId());
    event.SetNode(node);
    event.SetEdge(edge);
    event.SetPosition(wxPoint(int(x), int(y)));
    event.SetEventObject(canvas);
    canvas->GetEventHandler()->ProcessEvent(event);

    return event.IsAllowed();
}

void GraphElementHandler::Select(wxShape *shape, bool select, int keys)
{
    GraphElement *element = GetElement(shape);

    if ((keys & KEY_CTRL) == 0) {
        Graph *graph = element->GetGraph();
        Graph::iterator i, j, end;

        tie(i, end) = graph->GetSelection();

        while (i != end) {
            j = i++;
            if (&*j != element) {
                j->Unselect();
                select = true;
            }
        }
    }

    if (select)
        element->Select();
    else
        element->Unselect();
}

// ----------------------------------------------------------------------------
// Handler to give nodes the default behaviour
// ----------------------------------------------------------------------------

class GraphNodeHandler: public GraphElementHandler
{
public:
    typedef GraphEvent::NodeList NodeList;

    GraphNodeHandler(wxShapeEvtHandler *prev);

    void OnLeftClick(double x, double y, int keys, int attachment);
    void OnLeftDoubleClick(double x, double y, int keys, int attachment);
    void OnRightClick(double x, double y, int keys, int attachment);

    void OnBeginDragLeft(double x, double y, int keys, int attachment);
    void OnDragLeft(bool draw, double x, double y, int keys, int attachment);
    void OnEndDragLeft(double x, double y, int keys, int attachment);

    void OnBeginDragRight(double x, double y, int keys, int attachment);
    void OnDragRight(bool draw, double x, double y, int keys, int attachment);
    void OnEndDragRight(double x, double y, int keys, int attachment);

    void OnBeginDrag(int mode, double x, double y);
    void OnDrag(int mode, bool draw, double x, double y);
    void OnEndDrag(int mode, double x, double y);

    void OnEraseContents(wxDC& dc)  { GraphElementHandler::OnErase(dc); }
    void OnErase(wxDC& dc)          { wxShapeEvtHandler::OnErase(dc); }

    void OnDraw(wxDC& dc);

    void OnSizingDragLeft(wxControlPoint* pt, bool draw, double x, double y, int keys, int attachment);
    void OnSizingEndDragLeft(wxControlPoint* pt, double x, double y, int keys, int attachment);

    inline GraphNode *GetNode() const;

    enum {
        Drag_Move = GraphCtrl::Drag_Move,
        Drag_Connect = GraphCtrl::Drag_Connect
    };

private:
    NodeList m_sources;
    GraphNode *m_target;
    wxPoint m_offset;
};

GraphNodeHandler::GraphNodeHandler(wxShapeEvtHandler *prev)
  : GraphElementHandler(prev),
    m_target(NULL)
{
}

void GraphNodeHandler::OnLeftClick(double x, double y, int keys, int)
{
    HandleClick(Evt_Graph_Node_Click, x, y, keys);
}

void GraphNodeHandler::OnLeftDoubleClick(double x, double y, int keys, int)
{
    HandleDClick(Evt_Graph_Node_Activate, x, y, keys);
}

void GraphNodeHandler::OnRightClick(double x, double y, int keys, int)
{
    HandleRClick(Evt_Graph_Node_Menu, x, y, keys);
}

GraphNode *GraphNodeHandler::GetNode() const
{
    return tt_solutions::GetNode(GetShape());
}

void GraphNodeHandler::OnBeginDragLeft(double x, double y, int, int)
{
    OnBeginDrag(GraphCtrl::GetLeftDragMode(), x, y);
}

void GraphNodeHandler::OnDragLeft(bool draw, double x, double y, int, int)
{
    OnDrag(GraphCtrl::GetLeftDragMode(), draw, x, y);
}

void GraphNodeHandler::OnEndDragLeft(double x, double y, int, int)
{
    OnEndDrag(GraphCtrl::GetLeftDragMode(), x, y);
}

void GraphNodeHandler::OnBeginDragRight(double x, double y, int, int)
{
    OnBeginDrag(GraphCtrl::GetRightDragMode(), x, y);
}

void GraphNodeHandler::OnDragRight(bool draw, double x, double y, int, int)
{
    OnDrag(GraphCtrl::GetRightDragMode(), draw, x, y);
}

void GraphNodeHandler::OnEndDragRight(double x, double y, int, int)
{
    OnEndDrag(GraphCtrl::GetRightDragMode(), x, y);
}

void GraphNodeHandler::OnBeginDrag(int mode, double x, double y)
{
    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);

    m_target = NULL;
    m_offset = wxPoint(int(shape->GetX() - x), int(shape->GetY() - y));

    if (!shape->Selected()) {
        Select(shape, true, 0);
        canvas->Update();
    }

    OnDrag(mode, true, x, y);
    canvas->CaptureMouse();
}

void GraphNodeHandler::OnDrag(int mode, bool draw, double x, double y)
{
    bool hasNoEntry = m_target && m_sources.empty();
    int attachment = 0;

    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);
    Graph *graph = canvas->GetGraph();

    if (draw && (mode & Drag_Connect) != 0) {
        int new_attachment;
        wxShape *sh =
            canvas->FindFirstSensitiveShape(x, y, &new_attachment, OP_ALL);
        GraphNode *target;

        if (sh != NULL && sh != shape)
            target = wxDynamicCast(GetElement(sh), GraphNode);
        else
            target = NULL;

        if (target && target != m_target) {
            Graph::node_iterator it, end;
            m_sources.clear();

            for (tie(it, end) = graph->GetSelectionNodes(); it != end; ++it) {
                if (&*it != target) {
                    GraphNode::iterator i, iend;

                    for (tie(i, iend) = it->GetEdges(); i != iend; ++i) {
                        GraphEdge::iterator j, jend;

                        for (tie(j, jend) = i->GetNodes(); j != jend; ++j)
                            if (&*j == target)
                                break;

                        if (j != jend)
                            break;
                    }

                    if (i == iend)
                        m_sources.push_back(&*it);
                }
            }

            if (!m_sources.empty()) {
                GraphEvent event(Evt_Graph_Connect_Feedback);
                event.SetNode(GetNode());
                event.SetTarget(target);
                event.SetSources(m_sources);
                event.SetPosition(wxPoint(int(x), int(y)));
                graph->SendEvent(event);

                if (!event.IsAllowed())
                    m_sources.clear();
            }
        }

        m_target = target;
    }

    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);

    wxPen dottedPen(*wxBLACK, 1, wxDOT);
    dc.SetLogicalFunction(OGLRBLF);
    dc.SetPen(dottedPen);

    bool needNoEntry = m_target && m_sources.empty();

    if (needNoEntry && !hasNoEntry) {
        canvas->ReleaseMouse();
        canvas->SetCursor(wxCURSOR_NO_ENTRY);
        canvas->CaptureMouse();
    }
    else if (!needNoEntry && hasNoEntry) {
        canvas->ReleaseMouse();
        canvas->SetCursor(wxCURSOR_DEFAULT);
        canvas->CaptureMouse();
    }

    if ((mode & Drag_Connect) != 0 && m_target) {
        NodeList::iterator it;

        for (it = m_sources.begin(); it != m_sources.end(); ++it) {
            double xp, yp;
            (*it)->GetShape()->GetAttachmentPosition(attachment, &xp, &yp);
            dc.DrawLine(wxCoord(xp), wxCoord(yp), wxCoord(x), wxCoord(y));
        }
    }
    else if ((mode & Drag_Move) != 0) {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        Graph::node_iterator it, end;
        double shapeX = shape->GetX();
        double shapeY = shape->GetY();

        for (tie(it, end) = graph->GetSelectionNodes(); it != end; ++it) {
            wxShape *sh = it->GetShape();
            double xx = x - shapeX + sh->GetX() + m_offset.x;
            double yy = y - shapeY + sh->GetY() + m_offset.y;

            canvas->Snap(&xx, &yy);
            double w, h;
            sh->GetBoundingBoxMax(&w, &h);
            sh->OnDrawOutline(dc, xx, yy, w, h);
        }
    }
    else if ((mode && Drag_Connect) != 0) {
        Graph::node_iterator it, end;

        for (tie(it, end) = graph->GetSelectionNodes(); it != end; ++it) {
            double xp, yp;
            it->GetShape()->GetAttachmentPosition(attachment, &xp, &yp);
            dc.DrawLine(wxCoord(xp), wxCoord(yp), wxCoord(x), wxCoord(y));
        }
    }
}

void GraphNodeHandler::OnEndDrag(int mode, double x, double y)
{
    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);
    Graph *graph = canvas->GetGraph();
    canvas->ReleaseMouse();

    if ((mode & Drag_Connect) != 0 && m_target) {
        if (m_sources.empty()) {
            canvas->SetCursor(wxCURSOR_DEFAULT);
        }
        else {
            GraphEvent event(Evt_Graph_Connect);
            event.SetNode(GetNode());
            event.SetTarget(m_target);
            event.SetSources(m_sources);
            event.SetPosition(wxPoint(int(x), int(y)));
            graph->SendEvent(event);

            if (event.IsAllowed()) {
                NodeList::iterator it;

                for (it = m_sources.begin(); it != m_sources.end(); ++it)
                    graph->Add(**it, *m_target);
            }

            m_target = NULL;
            m_sources.clear();
        }
    }
    else if ((mode & Drag_Move) != 0) {
        wxPoint ptOffset = wxPoint(int(x), int(y)) + m_offset -
                           GetNode()->GetPosition();
        Graph::node_iterator it, end;

        for (tie(it, end) = graph->GetSelectionNodes(); it != end; ++it)
            it->SetPosition(it->GetPosition() + ptOffset);
    }
}

void GraphNodeHandler::OnDraw(wxDC& dc)
{
    GraphNode *node = GetNode();
    dc.SetFont(node->GetFont());
    node->OnDraw(dc);
}

void GraphNodeHandler::OnSizingDragLeft(wxControlPoint* pt, bool draw,
                                        double x, double y,
                                        int keys, int attachment)
{
    GraphElementHandler::OnSizingDragLeft(pt, draw, x, y, keys, attachment);
}

void GraphNodeHandler::OnSizingEndDragLeft(wxControlPoint* pt,
                                           double x, double y,
                                           int keys, int attachment)
{
    GraphNode *node = GetNode();
    wxShape *shape = GetShape();
    wxDiagram *diagram = shape->GetCanvas()->GetDiagram();
    node->Refresh();
    shape->Show(false);
    diagram->SetQuickEditMode(true);
    GraphElementHandler::OnSizingEndDragLeft(pt, x, y, keys, attachment);
    shape->Show(true);
    diagram->SetQuickEditMode(false);
    node->SetSize(node->GetSize());
}

// ----------------------------------------------------------------------------
// Handler for edges
// ----------------------------------------------------------------------------

class GraphEdgeHandler: public GraphElementHandler
{
public:
    GraphEdgeHandler(wxShapeEvtHandler *prev);

protected:
    wxRect GetEraseRect() const;
    inline wxLineShape *GetShape() const;
};

GraphEdgeHandler::GraphEdgeHandler(wxShapeEvtHandler *prev)
  : GraphElementHandler(prev)
{
}

wxLineShape *GraphEdgeHandler::GetShape() const
{
    return static_cast<wxLineShape*>(GraphElementHandler::GetShape());
};

wxRect GraphEdgeHandler::GetEraseRect() const
{
    wxRect rc = GraphHandler::GetEraseRect();

    wxLineShape *line = GetShape();
    GraphEdge *edge = GetEdge(line);

    wxList& arrows = line->GetArrows();
    wxList::iterator it;
    int size = 0;

    for (it = arrows.begin(); it != arrows.end(); ++it) {
        wxArrowHead *head = static_cast<wxArrowHead*>(*it);
        size = max(size, int(head->GetSize()));
    }

    rc.Inflate(size / 2 + edge->GetLineWidth());

    return rc;
}

// ----------------------------------------------------------------------------
// GraphNodeShape
// ----------------------------------------------------------------------------

class GraphNodeShape : public wxRectangleShape
{
public:
    bool GetPerimeterPoint(double x1, double y1,
                           double x2, double y2,
                           double *x3, double *y3);
};

bool GraphNodeShape::GetPerimeterPoint(double x1, double y1,
                                       double x2, double y2,
                                       double *x3, double *y3)
{
    GraphNode *node = GetNode(this);
    wxPoint pt1 = wxPoint(int(x1), int(y1));
    wxPoint pt2 = wxPoint(int(x2), int(y2));
    wxPoint inside, outside;

    if (node->GetBounds().Contains(pt1)) {
        inside = pt1;
        outside = pt2;
    } else {
        inside = pt2;
        outside = pt1;
    }

    wxPoint pt = GetNode(this)->GetPerimeterPoint(inside, outside);

    *x3 = pt.x;
    *y3 = pt.y;

    return true;
}

} // namespace

// ----------------------------------------------------------------------------
// GraphDiagram
// ----------------------------------------------------------------------------

namespace impl {

class GraphDiagram : public wxDiagram
{
public:
    void AddShape(wxShape *shape, wxShape *addAfter = NULL);
    void InsertShape(wxShape *shape);
    void SetEventHandler(wxShape *shape);
    void Redraw(wxDC& dc);
};

// The custom behaviour of the wxShapes is achieved using wxShapeEvtHandler
// objects rather than by overriding wxShape methods. This allows any old
// wxShape to be used.
//
void GraphDiagram::SetEventHandler(wxShape *shape)
{
    wxShapeEvtHandler *handler;
    void *data = shape->GetClientData();

    if (shape->GetClassInfo() == CLASSINFO(wxControlPoint))
        handler = new ControlPointHandler(shape);
    else if (wxDynamicCast(data, GraphNode))
        handler = new GraphNodeHandler(shape);
    else if (wxDynamicCast(data, GraphElement))
        handler = new GraphEdgeHandler(shape);
    else
        handler = new GraphHandler(shape);

    shape->SetEventHandler(handler);
}

void GraphDiagram::AddShape(wxShape *shape, wxShape *addAfter)
{
    SetEventHandler(shape);
    wxDiagram::AddShape(shape, addAfter);
}

void GraphDiagram::InsertShape(wxShape *shape)
{
    SetEventHandler(shape);
    wxDiagram::InsertShape(shape);
}

// Override Redraw since the default method displays a busy cursor which
// flashes on and off during panning.
//
void GraphDiagram::Redraw(wxDC& dc)
{
    if (m_shapeList) {
        wxList::iterator it;

        for (it = m_shapeList->begin(); it != m_shapeList->end(); ++it) {
            wxShape *object = static_cast<wxShape*>(*it);
            if (!object->GetParent())
                object->Draw(dc);
        }
    }
}

} // namespace impl

// ----------------------------------------------------------------------------
// iterator
// ----------------------------------------------------------------------------

namespace impl {

class GraphIteratorImpl
{
public:
    virtual ~GraphIteratorImpl() { }

    virtual void inc() = 0;
    virtual void dec() = 0;
    virtual bool eq(const GraphIteratorImpl& other) const = 0;
    virtual GraphElement *get() const = 0;
    virtual GraphIteratorImpl *clone() const = 0;
};

GraphIteratorBase::GraphIteratorBase(const GraphIteratorBase& it)
  : m_impl(it.m_impl ? it.m_impl->clone() : NULL)
{
}

GraphIteratorBase::~GraphIteratorBase()
{
    delete m_impl;
}

GraphElement& GraphIteratorBase::operator*() const
{
    return *m_impl->get();
}

GraphIteratorBase& GraphIteratorBase::operator=(const GraphIteratorBase& it)
{
    if (&it != this) {
        delete m_impl;
        m_impl = it.m_impl ? it.m_impl->clone() : NULL;
    }
    return *this;
}

GraphIteratorBase& GraphIteratorBase::operator++()
{
    m_impl->inc();
    return *this;
}

GraphIteratorBase& GraphIteratorBase::operator--()
{
    m_impl->dec();
    return *this;
}

bool GraphIteratorBase::operator ==(const GraphIteratorBase& it) const
{
    return (m_impl == NULL && it.m_impl == NULL) ||
           (m_impl != NULL && it.m_impl != NULL && m_impl->eq(*it.m_impl));
}

} // namespace impl

namespace {

class ListIterImpl : public GraphIteratorImpl
{
public:
    ListIterImpl(const wxList::iterator& begin,
                 const wxList::iterator& end,
                 wxClassInfo *classinfo,
                 int which = All,
                 const GraphNode *node = NULL)
      : m_it(begin),
        m_end(end),
        m_classinfo(classinfo == CLASSINFO(GraphElement) ? NULL : classinfo),
        m_which(which),
        m_node(node)
    {
        while (m_it != m_end && !filter())
            ++m_it;
    }

    bool filter()
    {
        GraphElement *element = get();

        if (!element)
            return false;

        if (m_which == Selected && !element->IsSelected())
            return false;

        if (m_classinfo && !element->IsKindOf(m_classinfo))
            return false;

        if (m_node && (m_which == InEdges || m_which == OutEdges))
        {
            GraphEdge *edge = wxDynamicCast(element, GraphEdge);
            if (!edge)
                return false;
            if (m_which == InEdges)
                return edge->GetTo() == m_node;
            else
                return edge->GetFrom() == m_node;
        }

        return true;
    }

    void inc()
    {
        do {
            ++m_it;
        }
        while (m_it != m_end && !filter());
    }

    void dec()
    {
        do {
            --m_it;
        }
        while (!filter());
    }

    bool eq(const GraphIteratorImpl& other) const
    {
        return typeid(other) == typeid(ListIterImpl) &&
               m_it == static_cast<const ListIterImpl&>(other).m_it;
    }

    GraphElement *get() const
    {
        return GetElement(wxStaticCast(*m_it, wxShape));
    }

    ListIterImpl *clone() const
    {
        return new ListIterImpl(*this);
    }

private:
    wxList::iterator m_it;
    wxList::iterator m_end;
    wxClassInfo *m_classinfo;
    int m_which;
    const GraphNode *m_node;
};

class PairIterImpl : public GraphIteratorImpl
{
public:
    PairIterImpl(wxLineShape *line, wxClassInfo *classinfo, bool end)
      : m_line(line),
        m_classinfo(classinfo == CLASSINFO(GraphElement) ? NULL : classinfo)
    {
        if (end) {
            m_pos = 3;
        } else {
            m_pos = 0;
            inc();
        }
    }

    bool filter()
    {
        GraphElement *element = get();
        return element && (!m_classinfo || element->IsKindOf(m_classinfo));
    }

    void inc()
    {
        do {
            m_pos++;
        }
        while (m_pos < 3 && !filter());
    }

    void dec()
    {
        do {
            m_pos--;
        }
        while (m_pos > 0 && !filter());
    }

    bool eq(const GraphIteratorImpl& other) const
    {
        if (typeid(other) != typeid(PairIterImpl))
            return false;
        const PairIterImpl& oth = static_cast<const PairIterImpl&>(other);
        return oth.m_line == m_line && oth.m_pos == m_pos;
    }

    GraphElement *get() const
    {
        return GetElement(m_pos == 1 ? m_line->GetFrom() : m_line->GetTo());
    }

    PairIterImpl *clone() const
    {
        return new PairIterImpl(*this);
    }

private:
    wxLineShape *m_line;
    wxClassInfo *m_classinfo;
    int m_pos;
};

} // namespace

// ----------------------------------------------------------------------------
// Graph
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(Graph, wxEvtHandler)

namespace {

const wxChar *TAGGRAPH  = _T("graph");
const wxChar *TAGFONT   = _T("font");
const wxChar *TAGSNAP   = _T("snap");
const wxChar *TAGGRID   = _T("grid");
const wxChar *TAGBOUNDS = _T("bounds");

class GraphInfo : public wxObject
{
public:
    GraphInfo()
    { }

    GraphInfo(const wxFont& font, const wxPoint& offset)
      : m_font(font), m_offset(offset)
    { }

    wxFont  GetFont()   const { return m_font; }
    wxPoint GetOffset() const { return m_offset; }

private:
    wxFont m_font;
    wxPoint m_offset;
};

} // namespace

Graph::Graph(wxEvtHandler *handler)
  : m_diagram(new GraphDiagram),
    m_handler(handler),
    m_dpi(GetScreenDPI())
{
    New();
}

Graph::~Graph()
{
    New();
    GraphCtrl *ctrl = GetCtrl();

    if (ctrl) {
        m_diagram->SetCanvas(NULL);
        ctrl->SetGraph(NULL);
    }
    else {
        delete m_diagram->GetCanvas();
    }

    delete m_diagram;
}

void Graph::New()
{
    iterator it, end;
    for (tie(it, end) = GetElements(); it != end; )
        delete &*it++;

    m_diagram->DeleteAllShapes();

    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    if (canvas) {
        canvas->SetFont(DefaultFont());
        wxStaticCast(canvas, GraphCanvas)->ScrollTo(wxPoint(0, 0), false);
    }

    SetGridSpacing(m_dpi.y / 18);

    RefreshBounds();
}

void Graph::SetEventHandler(wxEvtHandler *handler)
{
    m_handler = handler;
}

wxEvtHandler *Graph::GetEventHandler() const
{
    return m_handler;
}

void Graph::SendEvent(wxEvent& event)
{
    if (m_handler) {
        event.SetEventObject(this);
        m_handler->ProcessEvent(event);
    }
}

wxRect Graph::GetBounds() const
{
    if (m_rcBounds.IsEmpty()) {
        const_node_iterator it, end;

        for (tie(it, end) = GetNodes(); it != end; ++it)
            m_rcBounds.Union(it->GetBounds());
    }

    return m_rcBounds;
}

void Graph::RefreshBounds()
{
    GraphCanvas *canvas = GetCanvas();
    if (canvas)
        canvas->SetCheckBounds();
    m_rcBounds = wxRect();
}

void Graph::SetCanvas(GraphCanvas *canvas)
{
    GraphCtrl *oldctrl = GetCtrl();
    wxShapeCanvas *oldcanvas = m_diagram->GetCanvas();

    if (canvas == oldcanvas || (!canvas && !oldctrl))
        return;

    m_diagram->SetCanvas(canvas);
    canvas = GetCanvas();

    canvas->SetGraph(this);

    if (oldcanvas)
        canvas->SetFont(oldcanvas->GetFont());

    wxList::iterator it, end;
    wxList *list = m_diagram->GetShapeList();

    for (it = list->begin(); it != list->end(); ++it)
        wxStaticCast(*it, wxShape)->SetCanvas(canvas);

    if (!oldctrl)
        delete oldcanvas;
}

GraphCanvas *Graph::GetCanvas() const
{
    wxShapeCanvas *canvas = m_diagram->GetCanvas();

    if (!canvas) {
        GraphCanvas *gcanvas = new GraphCanvas;
        gcanvas->SetGraph(const_cast<Graph*>(this));
        m_diagram->SetCanvas(gcanvas);
        gcanvas->SetDiagram(m_diagram);
        return gcanvas;
    }
    else {
        return wxStaticCast(canvas, GraphCanvas);
    }
}

GraphCtrl *Graph::GetCtrl() const
{
    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    return canvas ? wxDynamicCast(canvas->GetParent(), GraphCtrl) : NULL;
}

wxList *Graph::GetShapeList() const
{
    return m_diagram->GetShapeList();
}

void Graph::SetFont(const wxFont& font)
{
    GetCanvas()->SetFont(font);
}

wxFont Graph::GetFont() const
{
    return GetCanvas()->GetFont();
}

GraphNode *Graph::Add(GraphNode *node,
                      wxPoint pt,
                      wxSize size)
{
    GraphEvent event(Evt_Graph_Node_Add);
    event.SetNode(node);
    event.SetPosition(pt);
    SendEvent(event);

    if (event.IsAllowed())
        return DoAdd(node, pt, size);

    delete node;
    return NULL;
}

GraphNode *Graph::DoAdd(GraphNode *node,
                        wxPoint pt,
                        wxSize size)
{
    wxASSERT(node != NULL);
    wxShape *shape = node->EnsureShape();
    wxASSERT_MSG(!shape->GetCanvas(), _T("Node already inserted into graph"));

    m_diagram->AddShape(shape);
    node->SetPosition(pt);
    node->SetSize(size);

    return node;
}

GraphEdge *Graph::Add(GraphNode& from, GraphNode& to, GraphEdge *edge)
{
    GraphEvent event(Evt_Graph_Edge_Add);
    event.SetNode(&from);
    event.SetTarget(&to);
    event.SetEdge(edge);
    SendEvent(event);
    edge = event.GetEdge();
    GraphNode *src = event.GetNode();
    GraphNode *dest = event.GetTarget();
    wxASSERT(src != NULL && dest != NULL);

    if (event.IsAllowed())
        return DoAdd(*src, *dest, edge);

    delete edge;
    return NULL;
}

GraphEdge *Graph::DoAdd(GraphNode& from, GraphNode& to, GraphEdge *edge)
{
    if (!edge)
        edge = new GraphEdge;

    wxLineShape *line = edge->EnsureShape();
    wxASSERT_MSG(!line->GetCanvas(), _T("Edge already inserted into graph"));

    m_diagram->InsertShape(line);
    ShowLine(line, &from, &to);
    edge->Refresh();

    return edge;
}

void Graph::Delete(GraphElement *element)
{
    GraphNode *node = wxDynamicCast(element, GraphNode);

    if (node) {
        GraphEvent event(Evt_Graph_Node_Delete);
        event.SetNode(node);
        SendEvent(event);

        if (event.IsAllowed()) {
            GraphNode::iterator it, end;

            for (tie(it, end) = node->GetEdges(); it != end; ++it)
                Delete(&*it);

            if (node->GetEdges().first == end) {
                DoDelete(node);
                RefreshBounds();
            }
        }
    }
    else {
        GraphEdge *edge = wxStaticCast(element, GraphEdge);

        GraphEvent event(Evt_Graph_Edge_Delete);
        event.SetEdge(edge);
        SendEvent(event);

        if (event.IsAllowed()) {
            edge->GetShape()->Unlink();
            DoDelete(edge);
        }
    }
}

void Graph::DoDelete(GraphElement *element)
{
    wxShape *shape = element->GetShape();
    if (shape->GetCanvas()) {
        element->Refresh();
        if (shape->Selected())
            shape->Select(false);
    }
    m_diagram->RemoveShape(shape);
    delete element;
}

void Graph::Delete(const iterator_pair& range)
{
    iterator i, endi;
    tie(i, endi) = range;

    while (i != endi)
    {
        GraphElement *element = &*i;
        ++i;
        GraphNode *node = wxDynamicCast(element, GraphNode);

        if (node) {
            GraphEvent event(Evt_Graph_Node_Delete);
            event.SetNode(node);
            SendEvent(event);

            if (event.IsAllowed()) {
                GraphNode::iterator j, endj;
                tie(j, endj) = node->GetEdges();

                while (j != endj)
                {
                    GraphEdge *edge = &*j;
                    ++j;
                    if (i != endi && edge == &*i)
                        ++i;
                    Delete(edge);
                }

                if (node->GetEdges().first == endj) {
                    DoDelete(node);
                    RefreshBounds();
                }
            }
        }
        else {
            Delete(wxStaticCast(element, GraphEdge));
        }
    }
}

GraphIteratorImpl *Graph::IterImpl(
    const wxList::iterator& begin,
    const wxList::iterator& end,
    wxClassInfo *classinfo,
    int which)
{
    return new ListIterImpl(begin, end, classinfo, which);
}

size_t Graph::GetNodeCount() const
{
    const_iterator it, end;
    size_t count = 0;

    for (tie(it, end) = GetNodes(); it != end; ++it)
        count++;

    return count;
}

size_t Graph::GetElementCount() const
{
    const_iterator it, end;
    size_t count = 0;

    for (tie(it, end) = GetElements(); it != end; ++it)
        count++;

    return count;
}

size_t Graph::GetSelectionCount() const
{
    const_iterator it, end;
    size_t count = 0;

    for (tie(it, end) = GetSelection(); it != end; ++it)
        count++;

    return count;
}

size_t Graph::GetSelectionNodeCount() const
{
    const_iterator it, end;
    size_t count = 0;

    for (tie(it, end) = GetSelectionNodes(); it != end; ++it)
        count++;

    return count;
}

bool Graph::LayoutAll(const GraphNode *fixed, double ranksep, double nodesep)
{
    return Layout(GetNodes(), fixed, ranksep, nodesep);
}

bool Graph::Layout(const node_iterator_pair& range,
                   const GraphNode *fixed,
                   double ranksep,
                   double nodesep)
{
    wxString dot;

    // Create a dot file for all the nodes in the range and the edges that
    // connect that. To find the edges, first put all the nodes into a set,
    // then iterate over all the edges of the nodes, looking for the edges
    // which connect nodes in the set.
    dot << _T("digraph Project {\n");
    dot << _T("\tnodesep=") << nodesep << _T("\n");
    dot << _T("\tranksep=") << ranksep << _T("\n");
    dot << _T("\tnode [label=\"\", shape=box, fixedsize=true];\n");

    wxSize dpi = wxSize(int(Points::Inch), int(Points::Inch));
    bool findFixed = fixed == NULL;
    bool externalConnection = false;
    node_iterator i, endi;

    typedef multiset<const GraphNode*, ElementCompare> NodeSet;
    typedef multiset<const GraphEdge*, ElementCompare> EdgeSet;
    typedef set< pair<wxString, const GraphNode*> > RankSet;
    NodeSet nodeset;
    EdgeSet edgeset;
    RankSet rankset;

    // First put the nodes into a set. The ElementCompare functor puts them
    // into the order they appear on the screen, which avoids the nodes
    // being randomly reordered on screen.
    for (tie(i, endi) = range; i != endi; ++i)
        nodeset.insert(&*i);

    // Now iterate over all the edges of all the nodes
    for (NodeSet::iterator it = nodeset.begin(); it != nodeset.end(); ++it)
    {
        const GraphNode *node = *it;
        GraphNode::const_iterator j, endj;
        bool extCon = false;

        for (tie(j, endj) = node->GetEdges(); j != endj; ++j)
        {
            const GraphNode *n1 = j->GetFrom(), *n2 = j->GetTo();

            // looking for edges which connect nodes in the set
            if (nodeset.count(n1 != node ? n1 : n2)) {
                // each edge will be found twice, but only add it once
                if (n1 == node)
                    edgeset.insert(&*j);
            }
            else {
                extCon = true;
            }
        }

        // If the range is a subset of the whole graph, and one of the nodes
        // has an edge to another node outside the range, then hold that
        // node fixed when doing the layout. Otherwise just fix the top-left-
        // most node.
        if (findFixed &&
            (!fixed || (!externalConnection && extCon) ||
             (externalConnection == extCon &&
              node->GetPosition() < fixed->GetPosition())))
        {
            fixed = node;
            externalConnection = extCon;
        }

        wxSize size = node->GetSize<Points>();

        // add the node to the dot file
        dot << _T("\t") << NodeName(*node)
            << _T(" [width=\"") << double(size.x) / dpi.x
            << _T("\", height=\"") << double(size.y) / dpi.y
            << _T("\"]\n");

        if (!node->GetRank().empty())
            rankset.insert(make_pair(node->GetRank(), node));
    }

    nodeset.clear();

    wxString rank;
    for (RankSet::iterator it = rankset.begin(); it != rankset.end(); ++it)
    {
        if (it->first != rank) {
            if (!rank.empty())
                dot << _T("\t}\n");
            rank = it->first;
            dot << _T("\tsubgraph {\n\t\trank = same;\n");
        }
        dot << _T("\t\t") << NodeName(*it->second) << _T(";\n");
    }
    if (!rank.empty())
        dot << _T("\t}\n");

    rankset.clear();

    // Now add the edges. These are also sorted by ElementCompare into the
    // order they appear on the screen, to avoid the nodes being reordered
    // too much.
    for (EdgeSet::iterator it = edgeset.begin(); it != edgeset.end(); ++it)
        dot << _T("\t") << NodeName(*(*it)->GetFrom())
            << _T(" -> ") << NodeName(*(*it)->GetTo())
            << _T(";\n");

    dot << _T("}\n");

    edgeset.clear();

#ifdef NO_GRAPHVIZ
    wxLogError(_("No layout engine available"));
    return false;
#else // using graphviz
    Agraph_t *graph;
    bool ok;
    GVC_t *context;

    {
        class GraphVizContext
        {
        public:
            GraphVizContext() {
                context = gvContext();
            }
            ~GraphVizContext() {
                gvFreeContext(context);
            }
            GVC_t* get() {
                return context;
            }
        private:
            GVC_t *context;
        };

#ifdef SUPPRESS_GRAPHVIZ_MEMLEAKS
        // memory allocated during GraphVizContext creation is never freed so
        // VC++ debug CRT reports it as leaked which is not really true as this
        // is a one-time only allocation and, anyhow, we can do nothing about
        // it, so just suppress the CRT reports about it
        class NoLeakCheck
        {
        public:
            NoLeakCheck() : flags(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)) {
                _CrtSetDbgFlag(flags & ~_CRTDBG_ALLOC_MEM_DF);
            }
            ~NoLeakCheck() {
                _CrtSetDbgFlag(flags);
            }
        private:
            int flags;
        };

        NoLeakCheck noCheck;
#endif // SUPPRESS_GRAPHVIZ_MEMLEAKS

        static GraphVizContext theContext;
        context = theContext.get();

        // parse the dot file
        graph = agmemread(unconst(dot.mb_str()));
        wxCHECK(graph, false);

        // do the layout
        ok = gvLayout(context, graph, (char*)"dot") == 0;
    }

    if (ok)
    {
        double offsetX = 0;
        double offsetY = 0;

        if (fixed) {
            Agnode_t *n = agfindnode(graph, unconst(NodeName(*fixed).mb_str()));
            if (n) {
                point pos = ND_coord_i(n);
                double x = PS2INCH(pos.x) * dpi.x;
                double y = - PS2INCH(pos.y) * dpi.y;
                wxPoint pt = fixed->GetPosition<Points>();
                offsetX = pt.x - x;
                offsetY = pt.y - y;
            }
        }

        for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
        {
            point pos = ND_coord_i(n);
            int x = int(offsetX + PS2INCH(pos.x) * dpi.x);
            int y = int(offsetY - PS2INCH(pos.y) * dpi.y);
            GraphNode *node;
            if (sscanf(n->name, "n%p", &node) == 1)
                node->SetPosition<Points>(wxPoint(x, y));
        }

        gvFreeLayout(context, graph);
    }
    else {
        wxLogError(_(
"An error occured laying out the graph with dot.\
 Please check the installation of the graphviz library and\
 its configuration file 'PREFIX/lib/graphviz/config'"
                    ));
    }

    agclose(graph);
    return ok;
#endif // NO_GRAPHVIZ/using graphviz
}

void Graph::Select(const iterator_pair& range)
{
    iterator i, j, end;
    tie(i, end) = range;

    while (i != end) {
        j = i++;
        j->Select();
    }
}

void Graph::Unselect(const iterator_pair& range)
{
    iterator i, j, end;
    tie(i, end) = range;

    while (i != end) {
        j = i++;
        j->Unselect();
    }
}

void Graph::SetSnapToGrid(bool snap)
{
    m_diagram->SetSnapToGrid(snap);
}

bool Graph::GetSnapToGrid() const
{
    return m_diagram->GetSnapToGrid();
}

void Graph::SetGridSpacing(int spacing)
{
    if (spacing < 1)
        spacing = 1;

#ifdef oglHAVE_XY_GRID
    int xspacing = WXROUND(double(spacing) * m_dpi.x / m_dpi.y);
    if (xspacing < 1)
        xspacing = 1;
    m_diagram->SetGridSpacing(xspacing, spacing);
#else
    m_diagram->SetGridSpacing(spacing);
#endif

    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    if (canvas)
        canvas->Refresh();
}

wxSize Graph::GetGridSpacing() const
{
    wxSize spacing;
    double x, y;

#ifdef oglHAVE_XY_GRID
    m_diagram->GetGridSpacing(&x, &y);
#else
    x = y = m_diagram->GetGridSpacing();
#endif

    spacing.x = WXROUND(x);
    spacing.y = WXROUND(y);

    return spacing;
}

bool Graph::CanClear() const
{
    const_iterator_pair its = GetSelection();
    return its.first == its.second;
}

bool Graph::Serialise(wxOutputStream& stream, const iterator_pair& range)
{
    Archive archive;
    return Serialise(archive, range) && archive.Save(stream);
}

bool Graph::Serialise(Archive& archive, const iterator_pair& range)
{
    bool badfactory = false;
    wxRect rcBounds;

    Graph::iterator it, end;

    if (range == iterator_pair())
        tie(it, end) = GetElements();
    else
        tie(it, end) = range;

    for ( ; it != end; ++it) {
        Factory<GraphElement> factory(*it);

        if (factory) {
            wxString name = factory.GetName();
            wxString id = Archive::MakeId(&*it);

            Archive::Item *arc = archive.Put(name, id);
            wxASSERT(arc);

            if (!it->Serialise(*arc))
                archive.Remove(id);
            else
                rcBounds += it->GetBounds();
        }
        else {
            wxFAIL_MSG(_T("Define a Factory<type>::Impl instance for ") +
                       wxString::FromAscii(typeid(*it).name()));
            badfactory = true;
        }
    }

    Archive::Item *graph = archive.Put(TAGGRAPH, TAGGRAPH);
    if (!graph)
        graph = archive.Get(TAGGRAPH);

    GraphCanvas *canvas = GetCanvas();
    if (canvas)
        graph->Put(TAGFONT, canvas->GetFont());

    graph->Put(TAGGRID, GetGridSpacing<Twips>());
    graph->Put(TAGSNAP, GetSnapToGrid());
    graph->Put(TAGBOUNDS, Twips::From<Pixels>(rcBounds, GetDPI()));

    if (badfactory)
        wxLogError(_("Internal error, not all elements could be saved"));

    return true;
}

bool Graph::Deserialise(wxInputStream& stream)
{
    Archive archive;
    return archive.Load(stream) && Deserialise(archive);
}

bool Graph::Deserialise(Archive& archive)
{
    New();
    Archive::Item *item = archive.Get(TAGGRAPH);

    if (item) {
        wxFont font;
        if (item->Get(TAGFONT, font))
            SetFont(font);

        int spacing;
        if (item->Get(TAGGRID, spacing))
            SetGridSpacing<Twips>(spacing);

        bool snap;
        if (item->Get(TAGSNAP, snap))
            SetSnapToGrid(snap);

        if (item->GetInstance() == NULL)
            item->SetInstance(new GraphInfo, true);
    }

    bool ok = DeserialiseInto(archive, wxPoint());

    GraphCtrl *ctrl = GetCtrl();
    if (ctrl) {
        ctrl->SetZoom(100.0);
        ctrl->Home();
    }

    return ok;
}

bool Graph::DeserialiseInto(wxInputStream& stream, const wxPoint& pt)
{
    Archive archive;
    return archive.Load(stream) && DeserialiseInto(archive, pt);
}

bool Graph::DeserialiseInto(Archive& archive, const wxPoint& pt)
{
    Archive::Item *item = archive.Get(TAGGRAPH);
    GraphCanvas *canvas = GetCanvas();

    if (item && item->GetInstance() == NULL) {
        wxFont font;

        if (item->Get(TAGFONT, font)) {
            wxString curdesc = canvas->GetFont().GetNativeFontInfoDesc();
            wxString newdesc = font.GetNativeFontInfoDesc();

            if (newdesc == curdesc)
                font = wxFont();
        }

        wxRect rc;
        wxPoint offset;

        if (item->Get(TAGBOUNDS, rc)) {
            rc = Twips::To<Pixels>(rc, GetDPI());
            offset = pt - (rc.GetPosition() + rc.GetSize() / 2);
        }

        item->SetInstance(new GraphInfo(font, offset), true);
    }

    Archive::iterator it, end;

    for (tie(it, end) = archive.GetItems(SORT_ELEMENT); it != end; ++it) {
        wxString sortkey = it->first;
        Archive::Item *arc = it->second;

        wxString classname = arc->GetClass();
        Factory<GraphElement> factory(classname);

        if (factory) {
            GraphElement *element = factory.New();
            wxShape *shape = element->EnsureShape();

            m_diagram->AddShape(shape);

            if (element->Serialise(*arc))
                element->Layout();
            else
                Delete(element);
        }
    }

    return true;
}

wxPoint Graph::FindSpace(const wxSize& spacing, int columns)
{
    Graph::node_iterator it, end;
    wxRect rc;

    for (tie(it, end) = GetNodes(); it != end; ++it)
        if (it->GetEdgeCount())
            rc.Union(it->GetBounds());

    wxPoint pt(0, rc.IsEmpty() ? 0 : rc.GetBottom());
    pt += spacing / 2;

    return FindSpace(pt, spacing, columns);
}

wxPoint Graph::FindSpace(const wxPoint& position,
                         const wxSize& spacing,
                         int columns)
{
    if (columns < 1) {
        columns = 4;

        GraphCanvas *canvas = GetCanvas();
        if (canvas != NULL) {
            wxRect rc = canvas->GetClientRect();
            rc = canvas->ScreenToGraph(rc);
            columns = rc.GetWidth() / spacing.x;
            columns = wxMax(columns, 1);
        }
    }

    bitset<8192> grid;
    const int rows = grid.size() / columns;

    wxPoint offset = position;
    offset -= spacing / 2;
    offset = -offset;

    Graph::node_iterator it, end;

    for (tie(it, end) = GetNodes(); it != end; ++it) {
        wxRect rc = it->GetBounds();
        rc.Offset(offset);

        wxPoint pt1 = rc.GetTopLeft();
        pt1.x /= spacing.x;
        pt1.y /= spacing.y;
        pt1.x = between(pt1.x, 0, columns);
        pt1.y = between(pt1.y, 0, rows);

        wxPoint pt2 = rc.GetBottomRight();
        pt2.x = (pt2.x + spacing.x - 1) / spacing.x;
        pt2.y = (pt2.y + spacing.y - 1) / spacing.y;
        pt2.x = between(pt2.x, 0, columns);
        pt2.y = between(pt2.y, 0, rows);

        for (int y = pt1.y; y < pt2.y; y++) {
            for (int x = pt1.x; x < pt2.x; x++) {
                int i = y * columns + x;
                wxASSERT(i >= 0 && size_t(i) < grid.size());
                grid[i] = true;
            }
        }
    }

    for (size_t i = 0; i < grid.size(); i++)
        if (!grid[i])
            return position + wxSize(spacing.x * (i % columns),
                                     spacing.y * (i / columns));

    return position;
}

void Graph::Draw(wxDC *dc, const wxRect& clip) const
{
    if (!clip.IsEmpty())
        dc->SetClippingRegion(clip);
    m_rcDraw = clip;
    m_diagram->Redraw(*dc);
    m_rcDraw = wxRect();
}

// ----------------------------------------------------------------------------
// GraphCtrl
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphCtrl, wxControl)

BEGIN_EVENT_TABLE(GraphCtrl, wxControl)
    EVT_SIZE(GraphCtrl::OnSize)
    EVT_CHAR(GraphCtrl::OnChar)
    EVT_TIMER(wxID_ANY, GraphCtrl::OnTipTimer)
    EVT_MOTION(GraphCtrl::OnMouseMove)
    EVT_ENTER_WINDOW(GraphCtrl::OnMouseLeave)
    EVT_MOUSEWHEEL(GraphCtrl::OnMouseWheel)
END_EVENT_TABLE()

const wxChar GraphCtrl::DefaultName[] = _T("graphctrl");
int GraphCtrl::sm_leftDrag = GraphCtrl::Drag_Move;
int GraphCtrl::sm_rightDrag = GraphCtrl::Drag_Connect;

GraphCtrl::GraphCtrl(
        wxWindow *parent,
        wxWindowID winid,
        const wxPoint& pos,
        const wxSize& size,
        long style,
        const wxValidator& validator,
        const wxString& name)
  : wxControl(parent, winid, pos, size, style | wxWANTS_CHARS, validator, name),
    m_canvas(new GraphCanvas(this, winid, wxPoint(0, 0), size, 0)),
    m_graph(NULL),
    m_tiptimer(this),
    m_tipdelay(500),
    m_tipwin(NULL)
{
}

GraphCtrl::~GraphCtrl()
{
    SetGraph(NULL);
    delete m_canvas;
}

void GraphCtrl::SetGraph(Graph *graph)
{
    if (m_graph)
        m_graph->SetCanvas(NULL);

    m_graph = graph;
    m_canvas->SetGraph(graph);

    if (graph) {
        m_canvas->SetDiagram(graph->m_diagram);
        graph->SetCanvas(m_canvas);
    }
    else {
        m_canvas->SetDiagram(NULL);
    }

    Refresh();
}

wxSize GraphCtrl::GetDPI() const
{
    return GetScreenDPI();
}

void GraphCtrl::SetZoom(double percent)
{
    SetZoom(percent, wxPoint() + m_canvas->GetClientSize() / 2);
}

void GraphCtrl::SetZoom(double percent, const wxPoint& ptCentre)
{
    GraphEvent event(Evt_Graph_Ctrl_Zoom, m_canvas->GetId());
    event.SetZoom(percent);
    event.SetPosition(ptCentre);
    event.SetEventObject(m_canvas);
    m_canvas->GetEventHandler()->ProcessEvent(event);
    if (!event.IsAllowed())
        return;
    percent = event.GetZoom();
    wxPoint pt = event.GetPosition();

    wxClientDC dc(m_canvas);
    m_canvas->PrepareDC(dc);
    wxPoint ptGraph(dc.DeviceToLogicalX(pt.x), dc.DeviceToLogicalY(pt.y));

    double scale = max(1.0, min(500.0, percent)) / 100.0;
    m_canvas->SetScale(scale, scale);

    m_canvas->Refresh();
    m_canvas->SetCheckBounds();

    m_canvas->PrepareDC(dc);
    int x = dc.LogicalToDeviceX(ptGraph.x) - pt.x;
    int y = dc.LogicalToDeviceY(ptGraph.y) - pt.y;

    m_canvas->ScrollByOffset(x, y, false);
}

double GraphCtrl::GetZoom() const
{
    return m_canvas->GetScaleX() * 100.0;
}

wxPoint GraphCtrl::GetScrollPosition() const
{
    return m_canvas->GetScrollPosition();
}

void GraphCtrl::ScrollTo(const wxPoint& pt)
{
    m_canvas->ScrollTo(pt);
}

void GraphCtrl::ScrollTo(const GraphElement& element)
{
    m_canvas->ScrollTo(element.GetPosition());
}

void GraphCtrl::ScrollTo(int side)
{
    if (m_graph)
        m_canvas->ScrollTo(side);
}

void GraphCtrl::EnsureVisible(const GraphElement& element)
{
    m_canvas->EnsureVisible(element.GetBounds());
}

void GraphCtrl::Home()
{
    if (!m_graph)
        return;

    GraphNode *root = NULL;
    Graph::node_iterator it, end;
    for (tie(it, end) = m_graph->GetNodes(); it != end; ++it)
        if (!root || it->GetPosition() < root->GetPosition())
            root = &*it;

    if (m_canvas->GetCheckBounds())
        m_canvas->SetFits();

    wxRect rc = m_graph->GetBounds();
    wxPoint ptCentre(rc.x + rc.width / 2, rc.y + rc.height / 2);
    wxPoint ptScroll = m_canvas->GetScroll();
    m_canvas->ScrollTo(ptCentre, false);

    if (m_canvas->GetCheckBounds())
        if (m_canvas->CheckBounds())
            m_canvas->ScrollTo(ptCentre, false);

    if (root)
        m_canvas->EnsureVisible(root->GetBounds(), false);

    ptScroll -= m_canvas->GetScroll();
    m_canvas->ScrollWindow(ptScroll.x, ptScroll.y);
}

void GraphCtrl::Fit()
{
    if (!m_graph)
        return;

    wxSize cs = m_canvas->GetFullClientSize();
    wxRect rc = m_graph->GetBounds();
    rc.Inflate(m_canvas->GetMargin());

    double scalex =  100.0 * cs.x / rc.width;
    double scaley =  100.0 * cs.y / rc.height;
    double scale = max(min(min(scalex, scaley), 100.0), 1.0);

    if (GetZoom() == scale) {
        m_canvas->ScrollTo(wxTOP);
    }
    else {
        SetZoom(scale);
        m_canvas->CheckBounds();
        m_canvas->ScrollTo(wxTOP, false);
    }
}

void GraphCtrl::SetBorder(const wxSize& border)
{
    m_canvas->SetBorder(border);
}

wxSize GraphCtrl::GetBorder() const
{
    return m_canvas->GetBorder();
}

void GraphCtrl::SetBorderType(BorderType type)
{
    m_canvas->SetBorderType(type);
}

GraphCtrl::BorderType GraphCtrl::GetBorderType() const
{
    return BorderType(m_canvas->GetBorderType());
}

void GraphCtrl::SetMargin(const wxSize& margin)
{
    m_canvas->SetMargin(margin);
}

wxSize GraphCtrl::GetMargin() const
{
    return m_canvas->GetMargin();
}

wxPoint GraphCtrl::ScreenToGraph(const wxPoint& ptScreen) const
{
    return m_canvas->ScreenToGraph(wxRect(ptScreen, wxSize())).GetTopLeft();
}

wxPoint GraphCtrl::GraphToScreen(const wxPoint& ptGraph) const
{
    return m_canvas->GraphToScreen(wxRect(ptGraph, wxSize())).GetTopLeft();
}

wxWindow *GraphCtrl::GetCanvas() const
{
    return m_canvas;
}

void GraphCtrl::OnSize(wxSizeEvent&)
{
    m_canvas->SetSize(GetClientSize());
}

void GraphCtrl::OnChar(wxKeyEvent& event)
{
    if (m_graph) {
        int key = event.GetKeyCode();

        switch (key) {
            case WXK_UP:
                m_canvas->DoScroll(wxVERTICAL, wxEVT_SCROLLWIN_LINEUP);
                break;
            case WXK_DOWN:
                m_canvas->DoScroll(wxVERTICAL, wxEVT_SCROLLWIN_LINEDOWN);
                break;
            case WXK_LEFT:
                m_canvas->DoScroll(wxHORIZONTAL, wxEVT_SCROLLWIN_LINEUP);
                break;
            case WXK_RIGHT:
                m_canvas->DoScroll(wxHORIZONTAL, wxEVT_SCROLLWIN_LINEDOWN);
                break;
            case WXK_PAGEUP:
                m_canvas->DoScroll(wxVERTICAL, wxEVT_SCROLLWIN_PAGEUP);
                break;
            case WXK_PAGEDOWN:
                m_canvas->DoScroll(wxVERTICAL, wxEVT_SCROLLWIN_PAGEDOWN);
                break;
            case WXK_HOME:
                m_canvas->ScrollTo(wxTOP);
                break;
            case WXK_END:
                m_canvas->ScrollTo(wxBOTTOM);
                break;
        }
    }

    event.Skip();
}

void GraphCtrl::OnMouseWheel(wxMouseEvent& event)
{
    bool zoom = event.ControlDown();
    int lines = event.GetWheelRotation() / event.GetWheelDelta();

    if (zoom) {
        double factor = pow(2.0, lines / 10.0);
        SetZoom(GetZoom() * factor, event.GetPosition());
    }
    else {
        int orient = event.ShiftDown() ? wxHORIZONTAL : wxVERTICAL;
        m_canvas->DoScroll(orient, wxEVT_SCROLLWIN_LINEUP, 0, lines);
    }

    event.Skip();
}

void GraphCtrl::OnMouseLeave(wxMouseEvent& event)
{
    m_tiptimer.Stop();

    event.Skip();
}

void GraphCtrl::OnMouseMove(wxMouseEvent& event)
{
    if (m_graph)
        m_tiptimer.Start(m_tipdelay, true);

    event.Skip();
}

void GraphCtrl::OnTipTimer(wxTimerEvent&)
{
    if (m_graph && !m_tipwin && FindFocus() == this) {
        wxMouseState mouse = wxGetMouseState();
        wxPoint pt = ScreenToGraph(wxPoint(mouse.GetX(), mouse.GetY()));
        GraphNode *node = m_graph->HitTest(pt);

        if (node) {
            wxString tip = node->GetToolTip(pt);

            if (!tip.empty()) {
                wxRect rc = m_canvas->GraphToScreen(node->GetBounds());
                m_tipwin = new wxTipWindow(this, tip, INT_MAX, &m_tipwin, &rc);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// GraphElement
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(GraphElement, wxObject)

GraphElement::GraphElement(const wxColour& colour,
                           const wxColour& bgcolour,
                           int style)
  : m_colour(colour),
    m_bgcolour(bgcolour),
    m_style(style),
    m_shape(NULL)
{
}

GraphElement::~GraphElement()
{
    delete m_shape;
}

GraphElement::GraphElement(const GraphElement& element)
  : wxObject(element),
    wxClientDataContainer(element),
    m_colour(element.m_colour),
    m_bgcolour(element.m_bgcolour),
    m_style(element.m_style),
    m_shape(NULL)
{
}

GraphElement& GraphElement::operator=(const GraphElement& element)
{
    if (&element != this) {
        m_colour = element.m_colour;
        m_bgcolour = element.m_bgcolour;

        if (m_shape) {
            if (element.m_shape)
                SetStyle(element.GetStyle());
            else
                SetShape(NULL);
        }

        m_style = element.m_style;
    }

    return *this;
}

void GraphElement::SetShape(wxShape *shape)
{
    wxShapeCanvas *canvas = m_shape ? m_shape->GetCanvas() : NULL;
    wxShape *prev = NULL;
    double x = 0, y = 0;
    double w = 0, h = 0;
    bool sel = false;

    if (canvas) {
        Refresh();

        x = m_shape->GetX();
        y = m_shape->GetY();

        m_shape->GetBoundingBoxMin(&w, &h);

        sel = m_shape->Selected();
        if (sel)
            m_shape->Select(false);

        if (shape) {
            wxList *list = canvas->GetDiagram()->GetShapeList();
            wxObjectListNode *node = list->Find(m_shape)->GetPrevious();
            prev = node ? static_cast<wxShape*>(node->GetData()) : NULL;
        }

        canvas->RemoveShape(m_shape);
    }

    delete m_shape;

    m_shape = shape;
    m_style = Style_Custom;

    if (shape) {
        shape->SetClientData(this);

        if (canvas) {
            shape->SetX(x);
            shape->SetY(y);
            shape->SetSize(w, h);

            canvas->AddShape(shape, prev);

            if (sel)
                shape->Select(true);
        }

        UpdateShape();
        Refresh();
    }
}

wxShape *GraphElement::DoEnsureShape()
{
    wxShape *shape = GetShape();

    if (!shape) {
        SetStyle(GetStyle());
        shape = GetShape();
    }

    return shape;
}

Graph *GraphElement::GetGraph() const
{
    wxShapeCanvas *canvas = GetCanvas(m_shape);
    return canvas ? wxStaticCast(canvas, GraphCanvas)->GetGraph() : NULL;
}

wxSize GraphElement::GetDPI() const
{
    return GetScreenDPI();
}

void GraphElement::Refresh()
{
    wxShapeCanvas *canvas = GetCanvas(m_shape);
    if (canvas) {
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);
        m_shape->Erase(dc);
    }
}

void GraphElement::OnDraw(wxDC& dc)
{
    wxPen pen(GetPen());
    wxBrush brush(GetBrush());

    m_shape->SetPen(&pen);
    m_shape->SetBrush(&brush);

    m_shape->OnDraw(dc);
    m_shape->OnDrawContents(dc);

    m_shape->SetPen(NULL);
    m_shape->SetBrush(NULL);
}

void GraphElement::SetColour(const wxColour& colour)
{
    m_colour = colour;
    Refresh();
}

void GraphElement::SetBackgroundColour(const wxColour& colour)
{
    m_bgcolour = colour;
    Refresh();
}

void GraphElement::DoSelect(bool select)
{
    if (m_shape && m_shape->Selected() != select)
    {
        wxShapeCanvas *canvas = GetCanvas(m_shape);

        if (canvas) {
            wxClientDC dc(canvas);
            canvas->PrepareDC(dc);
            if (select) {
                m_shape->Select(true);
                m_shape->OnEraseControlPoints(dc);
            }
            else {
                m_shape->OnEraseControlPoints(dc);
                m_shape->Select(false);
            }
        }
    }
}

bool GraphElement::IsSelected() const
{
    return m_shape && m_shape->Selected();
}

wxRect GraphElement::GetBounds() const
{
    wxShape *shape = GetShape();
    wxRect rc;

    if (shape) {
        double width, height;
        shape->GetBoundingBoxMin(&width, &height);
        rc.x = WXROUND(shape->GetX() - width / 2.0);
        rc.y = WXROUND(shape->GetY() - height / 2.0);
        rc.width = WXROUND(width);
        rc.height = WXROUND(height);
    }

    return rc;
}

wxPoint GraphElement::GetPosition() const
{
    wxShape *shape = GetShape();
    wxPoint pt;

    if (shape) {
        pt.x = WXROUND(shape->GetX());
        pt.y = WXROUND(shape->GetY());
    }

    return pt;
}

wxSize GraphElement::GetSize() const
{
    wxShape *shape = GetShape();
    wxSize size;

    if (shape) {
        double width, height;
        shape->GetBoundingBoxMin(&width, &height);
        size.x = WXROUND(width);
        size.y = WXROUND(height);
    }

    return size;
}

bool GraphElement::Serialise(Archive::Item& arc)
{
    const GraphElement& def = Factory<GraphElement>(this).GetDefault();

    arc.Exch(_T("colour"), m_colour, def.m_colour);
    arc.Exch(_T("bgcolour"), m_bgcolour, def.m_bgcolour);
    arc.Exch(_T("style"), m_style, def.m_style);

    return true;
}

// ----------------------------------------------------------------------------
// GraphEdge
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphEdge, GraphElement)
Factory<GraphEdge>::Impl graphedgefactory(_T("edge"));

GraphEdge::GraphEdge(const wxColour& colour,
                     const wxColour& bgcolour,
                     int style)
  : GraphElement(colour, bgcolour, style),
    m_arrowsize(10),
    m_linewidth(1)
{
}

GraphEdge::~GraphEdge()
{
}

void GraphEdge::SetShape(wxLineShape *line)
{
    wxLineShape *old = GetShape();

    if (old && line) {
        ShowLine(line, GetFrom(), GetTo());
        old->Unlink();
    }

    GraphElement::SetShape(line);
}

wxLineShape *GraphEdge::GetShape() const
{
    return static_cast<wxLineShape*>(GraphElement::DoGetShape());
}

wxLineShape *GraphEdge::EnsureShape()
{
    return static_cast<wxLineShape*>(GraphElement::DoEnsureShape());
}

void GraphEdge::SetStyle(int style)
{
    wxLineShape *line = new wxLineShape;

    line->MakeLineControlPoints(2);
    line->Show(true);

    if (style == Style_Arrow)
        line->AddArrow(ARROW_ARROW, ARROW_POSITION_END, m_arrowsize);

    SetShape(line);
    GraphElement::SetStyle(style);
}

void GraphEdge::SetArrowSize(int size)
{
    m_arrowsize = size;

    wxLineShape *shape = GetShape();

    if (shape) {
        Refresh();

        wxList& arrows = shape->GetArrows();
        wxList::iterator it;

        for (it = arrows.begin(); it != arrows.end(); ++it)
            static_cast<wxArrowHead*>(*it)->SetSize(size);

        Refresh();
    }
}

bool GraphEdge::MoveFront()
{
    wxLineShape *line = GetShape();
    wxShapeCanvas *canvas = GetCanvas(line);

    if (!canvas)
        return false;

    wxDiagram *diagram = canvas->GetDiagram();
    wxList *list = diagram->GetShapeList();
    wxList::iterator it = list->end();

    while (it != list->begin()) {
        if (*--it == line) {
            list->erase(it);
            list->push_front(line);
            break;
        }
    }

    return true;
}

bool GraphEdge::Serialise(Archive::Item& arc)
{
    if (!GraphElement::Serialise(arc))
        return false;

    Archive& archive = arc.GetArchive();
    wxString idFrom, idTo;

    if (archive.IsStoring()) {
        archive.SortItem(arc, SORT_EDGE);
        idFrom = Archive::MakeId(GetFrom());
        idTo = Archive::MakeId(GetTo());
    }

    const GraphEdge& def = Factory<GraphEdge>(this).GetDefault();
    arc.Exch(_T("from"), idFrom);
    arc.Exch(_T("to"), idTo);
    arc.Exch(_T("arrowsize"), m_arrowsize, def.m_arrowsize);
    arc.Exch(_T("linewidth"), m_linewidth, def.m_linewidth);

    if (archive.IsExtracting()) {
        GraphNode *from = archive.GetInstance<GraphNode>(idFrom);
        GraphNode *to = archive.GetInstance<GraphNode>(idTo);

        if (!ShowLine(GetShape(), from, to) || !MoveFront())
            return false;
    }

    return true;
}

impl::GraphIteratorImpl *GraphEdge::IterImpl(
    GraphLineShape *line,
    wxClassInfo *classinfo,
    bool end) const
{
    return new PairIterImpl(line, classinfo, end);
}

size_t GraphEdge::GetNodeCount() const
{
    size_t count = 0;
    if (GetFrom())
        count++;
    if (GetTo())
        count++;
    return count;
}

GraphNode *GraphEdge::GetFrom() const
{
    wxLineShape *line = GetShape();
    return line ? GetNode(line->GetFrom()) : NULL;
}

GraphNode *GraphEdge::GetTo() const
{
    wxLineShape *line = GetShape();
    return line ? GetNode(line->GetTo()) : NULL;
}

// ----------------------------------------------------------------------------
// GraphNode
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphNode, GraphElement)
Factory<GraphNode>::Impl graphnodefactory(_T("node"));

GraphNode::GraphNode(const wxString& text,
                     const wxColour& colour,
                     const wxColour& bgcolour,
                     const wxColour& textcolour,
                     int style)
  : GraphElement(colour, bgcolour, style),
    m_textcolour(textcolour),
    m_text(text)
{
}

GraphNode::~GraphNode()
{
}

void GraphNode::DoSelect(bool select)
{
    wxShape *shape = GetShape();

    if (shape && shape->Selected() != select)
    {
        wxShapeCanvas *canvas = GetCanvas(shape);

        if (canvas) {
            wxClientDC dc(canvas);
            canvas->PrepareDC(dc);

            if (select) {
                shape->Select(true);
                shape->OnEraseControlPoints(dc);

                wxRect rc, bounds = GetBounds();
                Graph::node_iterator begin, it;

                tie(begin, it) = GetGraph()->GetNodes();

                while (it != begin && &*--it != this)
                    rc.Union(it->GetBounds().Intersect(bounds));

                if (!rc.IsEmpty()) {
                    rc.x = dc.LogicalToDeviceX(rc.x);
                    rc.y = dc.LogicalToDeviceY(rc.y);
                    rc.width = dc.LogicalToDeviceXRel(rc.width);
                    rc.height = dc.LogicalToDeviceYRel(rc.height);
                    canvas->RefreshRect(rc);
                }

                wxDiagram *diagram = canvas->GetDiagram();
                wxList *list = diagram->GetShapeList();
                list->remove(shape);
                list->push_back(shape);
            }
            else {
                shape->OnEraseControlPoints(dc);
                shape->Select(false);
            }
        }
    }
}

void GraphNode::SetShape(wxShape *shape)
{
    wxShape *old = GetShape();

    if (old && shape) {
        wxList& lines = old->GetLines();
        wxList::iterator it, end;

        for (it = lines.begin(), end = lines.end(); it != end; ++it) {
            wxLineShape *line = static_cast<wxLineShape*>(*it);

            if (line->GetFrom() == old)
                shape->AddLine(line, line->GetTo());
            else
                line->GetFrom()->AddLine(line, shape);
        }
    }

    GraphElement::SetShape(shape);

    if (old && shape) {
        wxList& lines = shape->GetLines();
        wxList::iterator it, end;

        for (it = lines.begin(), end = lines.end(); it != end; ++it) {
            wxLineShape *line = static_cast<wxLineShape*>(*it);
            double x1, y1, x2, y2;
            line->FindLineEndPoints(&x1, &y1, &x2, &y2);
            line->SetEnds(x1, y1, x2, y2);
        }
    }
}

void GraphNode::SetStyle(int style)
{
    static const int triangle[][2] = {
        { 0, -1 }, { 1, 1 }, { -1, 1 }
    };
    static const int diamond[][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };

    wxShape *shape;

    switch (style) {
        case Style_Elipse:
            shape = new wxEllipseShape;
            break;
        case Style_Triangle:
            shape = CreatePolygon(WXSIZEOF(triangle), triangle);
            break;
        case Style_Diamond:
            shape = CreatePolygon(WXSIZEOF(diamond), diamond);
            break;
        default:
            shape = new GraphNodeShape;
            break;
    }

    shape->SetSize(100, 50);
    shape->Show(true);

    SetShape(shape);
    GraphElement::SetStyle(style);
}

void GraphNode::Layout()
{
    wxShapeCanvas *canvas = GetCanvas(GetShape());

    if (canvas) {
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);
        OnLayout(dc);
    }
}

void GraphNode::SetPosition(const wxPoint& pt)
{
    wxShape *shape = GetShape();
    wxShapeCanvas *canvas = GetCanvas(shape);

    if (canvas) {
        Graph *graph = GetGraph();
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);
        double x = pt.x, y = pt.y;
        canvas->Snap(&x, &y);

        GraphEvent event(Evt_Graph_Node_Move);
        event.SetPosition(wxPoint(int(x), int(y)));
        event.SetNode(this);
        graph->SendEvent(event);

        if (event.IsAllowed()) {
            wxPoint pt = event.GetPosition();
            shape->Erase(dc);
            shape->Move(dc, pt.x, pt.y, false);
            shape->Erase(dc);
            OnLayout(dc);
            graph->RefreshBounds();
        }
    }
}

void GraphNode::DoSetSize(wxDC& dc, const wxSize& size)
{
    wxShape *shape = GetShape();
    shape->Erase(dc);
    shape->SetSize(size.x, size.y);
    shape->ResetControlPoints();
    shape->MoveLinks(dc);
    shape->Erase(dc);
    GetGraph()->RefreshBounds();
}

void GraphNode::SetSize(const wxSize& size)
{
    wxShape *shape = GetShape();
    wxShapeCanvas *canvas = GetCanvas(shape);

    if (canvas) {
        GraphEvent event(Evt_Graph_Node_Size);
        event.SetSize(size);
        event.SetNode(this);
        GetGraph()->SendEvent(event);

        if (event.IsAllowed()) {
            wxClientDC dc(canvas);
            canvas->PrepareDC(dc);
            DoSetSize(dc, event.GetSize());
            OnLayout(dc);
        }
    }
}

void GraphNode::UpdateShape()
{
    wxShape *shape = GetShape();

    if (shape) {
        shape->AddText(m_text);
        shape->SetFont(&m_font);
        UpdateShapeTextColour();
        Layout();
        Refresh();
    }
}

void GraphNode::SetText(const wxString& text)
{
    m_text = text;

    wxShape *shape = GetShape();

    if (shape) {
        shape->AddText(m_text);
        Layout();
        Refresh();
    }
}

wxString GraphNode::GetToolTip(const wxPoint&) const
{
    return m_tooltip;
}

void GraphNode::SetFont(const wxFont& font)
{
    m_font = font;

    wxShape *shape = GetShape();

    if (shape) {
        shape->SetFont(&m_font);
        Layout();
        Refresh();
    }
}

wxFont GraphNode::GetFont() const
{
    if (!m_font.Ok()) {
        wxShapeCanvas *canvas = GetCanvas(GetShape());
        if (canvas)
            return canvas->GetFont();
    }
    return m_font;
}

void GraphNode::SetTextColour(const wxColour& colour)
{
    m_textcolour = colour;
    UpdateShapeTextColour();
    Refresh();
}

void GraphNode::UpdateShapeTextColour()
{
    wxShape *shape = GetShape();

    if (shape) {
        wxString name = wxTheColourDatabase->FindName(m_textcolour);
        if (name.empty()) {
            name.Printf(_T("RGB-%d-%d-%d"), m_textcolour.Red(),
                        m_textcolour.Green(), m_textcolour.Blue());
            wxTheColourDatabase->AddColour(name, m_textcolour);
        }
        shape->SetTextColour(name);
    }
}

wxList *GraphNode::GetLines() const
{
    return &GetShape()->GetLines();
}

impl::GraphIteratorImpl *GraphNode::IterImpl(
    const wxList::iterator& begin,
    const wxList::iterator& end,
    wxClassInfo *classinfo,
    int which) const
{
    return new ListIterImpl(begin, end, classinfo, which, this);
}

size_t GraphNode::GetEdgeCount() const
{
    wxList& list = GetShape()->GetLines();
    return list.size();
}

size_t GraphNode::GetInEdgeCount() const
{
    const_iterator it, end;
    size_t count = 0;

    for (tie(it, end) = GetInEdges(); it != end; ++it)
        count++;

    return count;
}

size_t GraphNode::GetOutEdgeCount() const
{
    const_iterator it, end;
    size_t count = 0;

    for (tie(it, end) = GetOutEdges(); it != end; ++it)
        count++;

    return count;
}

wxPoint GraphNode::GetPerimeterPoint(const wxPoint& inside,
                                     const wxPoint& outside) const
{
    wxRect b = GetBounds();
    wxPoint k = inside;
    wxPoint pt = outside;

    b.Inflate(1);

    int dx = pt.x - k.x;
    int dy = pt.y - k.y;

    if (dx != 0) {
        pt.x = pt.x < k.x ?  b.x : b.GetRight();
        pt.y = k.y + (pt.x - k.x) * dy / dx;
    }

    if (dy != 0 && (dx == 0 || pt.y < b.y || pt.y > b.GetBottom())) {
        pt.y = pt.y < k.y ? b.y : b.GetBottom();
        pt.x = k.x + (pt.y - k.y) * dx / dy;
    }

    return pt;
}

bool GraphNode::Serialise(Archive::Item& arc)
{
    if (!GraphElement::Serialise(arc))
        return false;

    Archive& archive = arc.GetArchive();
    wxPoint position;
    wxSize size;

    if (arc.IsStoring()) {
        archive.SortItem(arc, SORT_NODE);
        position = GetPosition<Twips>();
        size = GetSize<Twips>();
    }

    const GraphNode& def = Factory<GraphNode>(this).GetDefault();
    arc.Exch(_T("textcolour"), m_textcolour, def.m_textcolour);
    arc.Exch(_T("font"), m_font, def.m_font);
    arc.Exch(_T("text"), m_text, def.m_text);
    arc.Exch(_T("tooltip"), m_tooltip, def.m_tooltip);
    arc.Exch(_T("rank"), m_rank, def.m_rank);
    arc.Exch(_T("position"), position);
    arc.Exch(_T("size"), size);

    if (arc.IsExtracting()) {
        GraphInfo *info = archive.GetInstance<GraphInfo>(TAGGRAPH);
        if (info) {
            if (!m_font.IsOk())
                m_font = info->GetFont();
            position += info->GetOffset();
        }

        arc.SetInstance(this);
        SetPosition<Twips>(position);
        SetSize<Twips>(size);
    }

    return true;
}

} // namespace tt_solutions
