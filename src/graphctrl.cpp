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

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

namespace {

// sort order for the elements when loading
const wxChar *SORT_NODE = _T("1");
const wxChar *SORT_EDGE = _T("2");

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

bool ShowLine(GraphEdge *edge, GraphNode *from, GraphNode *to)
{
    if (!edge || !from || !to)
        return false;

    wxLineShape *line = edge->GetShape();
    wxShape *fromshape = from->GetShape();
    wxShape *toshape = to->GetShape();

    if (!line || !fromshape || !toshape)
        return false;

    fromshape->AddLine(line, toshape);

    double x1, y1, x2, y2;
    line->FindLineEndPoints(&x1, &y1, &x2, &y2);
    line->SetEnds(x1, y1, x2, y2);

    return true;
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
    m_edge(NULL)
{
}

GraphEvent::GraphEvent(const GraphEvent& event)
  : wxNotifyEvent(event),
    m_pos(event.m_pos),
    m_node(event.m_node),
    m_target(event.m_target),
    m_edge(event.m_edge)
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

    static const wxChar DefaultName[];

    void OnLeftClick(double x, double y, int keys);

    void OnDragLeft(bool draw, double x, double y, int keys);
    void OnBeginDragLeft(double x, double y, int keys);
    void OnEndDragLeft(double x, double y, int keys);

    void SetGraph(Graph *graph) { m_graph = graph; }
    Graph *GetGraph() const { return m_graph; }

    void OnScroll(wxScrollWinEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

    void PrepareDC(wxDC& dc);

    void AdjustScrollbars() { }

    void SetCheckBounds() { m_checkBounds = true; }
    void CheckBounds();

    void ScrollTo(const wxPoint& ptGraph, bool draw = true);
    wxPoint ScrollByOffset(int x, int y, bool draw = true);

    void EnsureVisible(const wxRect& rc);

    wxRect ScreenToGraph(const wxRect& rcScreen);
    wxRect GraphToScreen(const wxRect& rcGraph);

private:
    Graph *m_graph;
    bool m_isPanning;
    bool m_checkBounds;
    wxPoint m_ptDrag;
    wxPoint m_ptOrigin;
    wxSize m_sizeScrollbar;

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
END_EVENT_TABLE()

GraphCanvas::GraphCanvas(
        wxWindow *parent,
        wxWindowID id,
        const wxPoint& pos,
        const wxSize& size,
        long style,
        const wxString& name)
  : wxShapeCanvas(parent, id, pos, size, style, name),
    m_graph(NULL),
    m_isPanning(false),
    m_checkBounds(false)
{
    SetScrollRate(1, 1);
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

void GraphCanvas::OnScroll(wxScrollWinEvent& event)
{
    bool horz = event.GetOrientation() == wxHORIZONTAL;
    int type = event.GetEventType();
    int pos = event.GetPosition();
    int scroll = horz ? m_xScrollPosition : m_yScrollPosition;
    int size = horz ? m_virtualSize.x : m_virtualSize.y;
    int cs = horz ? GetClientSize().x : GetClientSize().y;

    if (type == wxEVT_SCROLLWIN_TOP)
        pos = 0;
    else if (type == wxEVT_SCROLLWIN_BOTTOM)
        pos = size;
    else if (type == wxEVT_SCROLLWIN_LINEUP)
        pos = scroll - 10;
    else if (type == wxEVT_SCROLLWIN_LINEDOWN)
        pos = scroll + 10;
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

// The scrollbars are sized to allow scrolling over the bounding rectangle of
// all the nodes currently in the graph. Panning allows scrolling beyond this
// region, and in this case the scrollbars adjust to include the current
// position. Either way the scrollbars always allow scrolling back to the
// graph origin, i.e. to have graph position (0, 0) in the top-left of the
// current viewport.
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
    if (m_checkBounds && !wxGetMouseState().LeftDown())
        CheckBounds();
}

void GraphCanvas::CheckBounds()
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxSize cs = GetClientSize() + m_sizeScrollbar;
    wxSize fullclient = cs;

    wxRect b = m_graph->GetBounds();
    b.x = dc.LogicalToDeviceX(b.x);
    b.y = dc.LogicalToDeviceY(b.y);
    b.width = dc.LogicalToDeviceXRel(b.width);
    b.height = dc.LogicalToDeviceYRel(b.height);

    int x = min(min(0, b.x) + m_xScrollPosition, m_ptOrigin.x);
    m_ptOrigin.x -= x;
    m_xScrollPosition -= x;

    int y = min(min(0, b.y) + m_yScrollPosition, m_ptOrigin.y);
    m_ptOrigin.y -= y;
    m_yScrollPosition -= y;

    bool needHBar = false, needVBar = false;
    bool done = false;

    while (!done) {
        done = true;

        if (m_xScrollPosition == 0 && m_ptOrigin.x == 0)
            m_virtualSize.x = b.GetRight();
        else
            m_virtualSize.x =
                max(max(cs.x, b.GetRight()) + m_xScrollPosition,
                    m_ptOrigin.x + cs.x);

        if (m_virtualSize.x > cs.x) {
            SetScrollbar(wxHORIZONTAL, m_xScrollPosition, cs.x,
                         m_virtualSize.x);
            GetClientSize(NULL, &cs.y);
            needHBar = true;
        }

        if (m_yScrollPosition == 0 && m_ptOrigin.y == 0)
            m_virtualSize.y = b.GetBottom();
        else
            m_virtualSize.y =
                max(max(cs.y, b.GetBottom()) + m_yScrollPosition,
                    m_ptOrigin.y + cs.y);

        if (m_virtualSize.y > cs.y) {
            SetScrollbar(wxVERTICAL, m_yScrollPosition, cs.y, m_virtualSize.y);
            GetClientSize(&cs.x, NULL);
            done = needVBar;
            needVBar = true;
        }
    }

    if (!needHBar)
        SetScrollbar(wxHORIZONTAL, 0, 0, 0);
    if (!needVBar)
        SetScrollbar(wxVERTICAL, 0, 0, 0);

    m_sizeScrollbar = fullclient - cs;

    m_checkBounds = false;
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

void GraphCanvas::ScrollTo(const wxPoint& ptGraph, bool draw)
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxSize cs = GetClientSize();
    int x = dc.LogicalToDeviceX(ptGraph.x) - cs.x / 2;
    int y = dc.LogicalToDeviceY(ptGraph.y) - cs.y / 2;

    ScrollByOffset(x, y, draw);
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

void GraphCanvas::EnsureVisible(const wxRect& rcGraph)
{
    wxClientDC dc(this);
    PrepareDC(dc);

    wxRect rc(dc.LogicalToDeviceX(rcGraph.x),
              dc.LogicalToDeviceY(rcGraph.y),
              dc.LogicalToDeviceXRel(rcGraph.width),
              dc.LogicalToDeviceYRel(rcGraph.height));

    wxRect rcClient = GetClientSize();

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
        ScrollByOffset(x, y);
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
};

GraphHandler::GraphHandler(wxShapeEvtHandler *prev)
  : wxShapeEvtHandler(prev, prev->GetShape())
{
}

void GraphHandler::OnErase(wxDC& dc)
{
    wxShape *shape = GetShape();

    if (shape->IsShown() && dc.IsKindOf(CLASSINFO(wxWindowDC)))
    {
        wxWindow *canvas = shape->GetCanvas();

        const wxPen *pen = shape->GetPen();
        int penWidth = pen ? pen->GetWidth() : 0;
        penWidth += 2;

        double sizeX, sizeY;
        shape->GetBoundingBoxMax(&sizeX, &sizeY);
        sizeX += 2.0 * penWidth + 4.0;
        sizeY += 2.0 * penWidth + 4.0;
        double x = shape->GetX() - sizeX / 2.0;
        double y = shape->GetY() - sizeY / 2.0;

        wxRect rc(dc.LogicalToDeviceX(WXROUND(x)),
                  dc.LogicalToDeviceY(WXROUND(y)),
                  dc.LogicalToDeviceXRel(WXROUND(sizeX)),
                  dc.LogicalToDeviceYRel(WXROUND(sizeY)));

        canvas->RefreshRect(rc);
    }
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

    void OnEraseContents(wxDC& dc)  { GraphElementHandler::OnErase(dc); }
    void OnErase(wxDC& dc)          { wxShapeEvtHandler::OnErase(dc); }

    void OnDraw(wxDC& dc);
    void OnDrawContents(wxDC&)      { }

    void OnSizingDragLeft(wxControlPoint* pt, bool draw, double x, double y, int keys, int attachment);
    void OnSizingEndDragLeft(wxControlPoint* pt, double x, double y, int keys, int attachment);

    inline GraphNode *GetNode() const;

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

void GraphNodeHandler::OnBeginDragLeft(double x, double y,
                                       int keys, int attachment)
{
    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);

    m_target = NULL;
    m_offset = wxPoint(int(shape->GetX() - x), int(shape->GetY() - y));

    if (!shape->Selected()) {
        Select(shape, true, keys);
        canvas->Update();
    }

    OnDragLeft(true, x, y, keys, attachment);
    canvas->CaptureMouse();
}

void GraphNodeHandler::OnDragLeft(bool draw, double x, double y,
                                  int, int attachment)
{
    bool hasNoEntry = m_target && m_sources.empty();
    attachment = 0;

    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);
    Graph *graph = canvas->GetGraph();

    if (draw) {
        int new_attachment;
        wxShape *sh =
            canvas->FindFirstSensitiveShape(x, y, &new_attachment, OP_ALL);
        GraphNode *target;

        if (sh != NULL && sh != shape)
            target = wxDynamicCast(GetElement(sh), GraphNode);
        else
            target = NULL;

        if (target != NULL && target != m_target) {
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

    if (m_target) {
        NodeList::iterator it;

        for (it = m_sources.begin(); it != m_sources.end(); ++it) {
            double xp, yp;
            (*it)->GetShape()->GetAttachmentPosition(attachment, &xp, &yp);
            dc.DrawLine(wxCoord(xp), wxCoord(yp), wxCoord(x), wxCoord(y));
        }
    }
    else {
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
}

void GraphNodeHandler::OnEndDragLeft(double x, double y, int, int)
{
    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);
    Graph *graph = canvas->GetGraph();
    canvas->ReleaseMouse();

    if (m_target) {
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
    else {
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
        handler = new GraphElementHandler(shape);
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
    GraphIteratorImpl(int flags, bool selectionOnly)
        : m_flags(flags), m_selectionOnly(selectionOnly)
    { }
    GraphIteratorImpl(const GraphIteratorImpl& other)
        : m_flags(other.m_flags), m_selectionOnly(other.m_selectionOnly)
    { }

    virtual ~GraphIteratorImpl() { }

    virtual void inc() = 0;
    virtual void dec() = 0;
    virtual bool eq(const GraphIteratorImpl& other) const = 0;
    virtual GraphElement *get() const = 0;
    virtual GraphIteratorImpl *clone() const = 0;

    enum Flags {
        AllElements, NodesOnly, EdgesOnly, InEdgesOnly, OutEdgesOnly, Pair
    };

    int GetFlags() const { return m_flags; }
    bool IsSelectionOnly() const { return m_selectionOnly; }

private:
    int m_flags;
    bool m_selectionOnly;
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
                 const wxList::iterator& end = wxList::iterator(),
                 int flags = AllElements,
                 bool selectionOnly = false,
                 const GraphNode *node = NULL)
      : GraphIteratorImpl(flags, selectionOnly),
        m_it(begin),
        m_end(end),
        m_node(node)
    {
        wxASSERT(flags == AllElements ||
                 flags == NodesOnly ||
                 flags == EdgesOnly ||
                 flags == InEdgesOnly ||
                 flags == OutEdgesOnly);

        wxASSERT((flags != InEdgesOnly && flags != OutEdgesOnly) || node);

        while (m_it != m_end && !filter())
            ++m_it;
    }

    ListIterImpl(const ListIterImpl& other)
      : GraphIteratorImpl(other),
        m_it(other.m_it),
        m_end(other.m_end)
    {
    }

    bool filter()
    {
        GraphElement *element = get();

        if (!element)
            return false;
        if (IsSelectionOnly() && !element->IsSelected())
            return false;

        int flags = GetFlags();

        if (flags == NodesOnly)
            return element->IsKindOf(CLASSINFO(GraphNode));

        if (flags == EdgesOnly || flags == InEdgesOnly
                || flags == OutEdgesOnly)
        {
            GraphEdge *edge = wxDynamicCast(element, GraphEdge);
            if (!edge)
                return false;
            if (flags == InEdgesOnly)
                return edge->GetTo() == m_node;
            if (flags == OutEdgesOnly)
                return edge->GetFrom() == m_node;
            return true;
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
        return other.GetFlags() == GetFlags() &&
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
    const GraphNode *m_node;
};

class PairIterImpl : public GraphIteratorImpl
{
public:
    PairIterImpl(wxLineShape *line, bool end)
      : GraphIteratorImpl(Pair, false), m_line(line)
    {
        if (end) {
            m_pos = 3;
        } else {
            m_pos = 0;
            inc();
        }
    }

    PairIterImpl(const PairIterImpl& other)
      : GraphIteratorImpl(other), m_line(other.m_line), m_pos(other.m_pos)
    {
    }

    void inc()
    {
        m_pos++;
        if (m_pos == 1 && m_line->GetFrom() == NULL)
            m_pos++;
        if (m_pos == 2 && m_line->GetTo() == NULL)
            m_pos++;
    }

    void dec()
    {
        m_pos--;
        if (m_pos == 2 && m_line->GetTo() == NULL)
            m_pos--;
        if (m_pos == 1 && m_line->GetFrom() == NULL)
            m_pos--;
    }

    bool eq(const GraphIteratorImpl& other) const
    {
        if (other.GetFlags() != GetFlags())
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
    int m_pos;
};

} // namespace

// ----------------------------------------------------------------------------
// Graph
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(Graph, wxEvtHandler)

namespace {

const wxChar *tagGRAPH  = _T("graph");
const wxChar *tagFONT   = _T("font");
const wxChar *tagSNAP   = _T("snap");
const wxChar *tagGRID   = _T("grid");
const wxChar *tagBOUNDS = _T("bounds");

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

Graph::Graph()
  : m_diagram(new GraphDiagram),
    m_handler(NULL),
    m_gridSpacing(1.0 / 18)
{
}

Graph::~Graph()
{
    wxASSERT_MSG(!m_diagram->GetCanvas(),
        _T("Can't delete a Graph while it is assigned to a GraphCtrl, ")
        _T("delete the GraphCtrl first or call GraphCtrl::SetGraph(NULL)"));

    Delete(GetElements());
    m_diagram->DeleteAllShapes();
    delete m_diagram;
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
            m_rcBounds += it->GetBounds();
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
    m_diagram->SetCanvas(canvas);

    if (canvas && m_dpi == wxSize()) {
        m_dpi = wxClientDC(canvas).GetPPI();
        if (m_gridSpacing)
            SetGridSpacing(int(m_gridSpacing * m_dpi.y));
    }

    wxList::iterator it, end;
    wxList *list = m_diagram->GetShapeList();

    for (it = list->begin(); it != list->end(); ++it)
        wxStaticCast(*it, wxShape)->SetCanvas(canvas);
}

GraphCanvas *Graph::GetCanvas() const
{
    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    return canvas ? wxStaticCast(canvas, GraphCanvas) : NULL;
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
    ShowLine(edge, &from, &to);
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

Graph::iterator_pair Graph::GetElements()
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();

    return make_pair(iterator(new ListIterImpl(begin)),
                     iterator(new ListIterImpl(end)));
}

Graph::node_iterator_pair Graph::GetNodes()
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::NodesOnly;

    return make_pair(
        node_iterator(new ListIterImpl(begin, end, flags)),
        node_iterator(new ListIterImpl(end, end, flags)));
}

Graph::iterator_pair Graph::GetSelection()
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::AllElements;

    return make_pair(iterator(new ListIterImpl(begin, end, flags, true)),
                     iterator(new ListIterImpl(end, end, flags, true)));
}

Graph::node_iterator_pair Graph::GetSelectionNodes()
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::NodesOnly;

    return make_pair(node_iterator(new ListIterImpl(begin, end, flags, true)),
                     node_iterator(new ListIterImpl(end, end, flags, true)));
}

Graph::const_iterator_pair Graph::GetElements() const
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();

    return make_pair(const_iterator(new ListIterImpl(begin)),
                     const_iterator(new ListIterImpl(end)));
}

Graph::const_node_iterator_pair Graph::GetNodes() const
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::NodesOnly;

    return make_pair(const_node_iterator(new ListIterImpl(begin, end, flags)),
                     const_node_iterator(new ListIterImpl(end, end, flags)));
}

Graph::const_iterator_pair Graph::GetSelection() const
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::AllElements;

    return make_pair(const_iterator(new ListIterImpl(begin, end, flags, true)),
                     const_iterator(new ListIterImpl(end, end, flags, true)));
}

Graph::const_node_iterator_pair Graph::GetSelectionNodes() const
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::NodesOnly;

    return make_pair(
            const_node_iterator(new ListIterImpl(begin, end, flags, true)),
            const_node_iterator(new ListIterImpl(end, end, flags, true)));
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

bool Graph::Layout(const node_iterator_pair& range)
{
    wxString dot;

    // Create a dot file for all the nodes in the range and the edges that
    // connect that. To find the edges, first put all the nodes into a set,
    // then iterate over all the edges of the nodes, looking for the edges
    // which connect nodes in the set.
    dot << _T("digraph Project {\n");
    dot << _T("\tnode [label=\"\", shape=box, fixedsize=true];\n");

    wxSize dpi = wxSize(Points::Inch, Points::Inch);
    const GraphNode *fixed = NULL;
    bool externalConnection = false;
    node_iterator i, endi;

    typedef multiset<const GraphNode*, ElementCompare> NodeSet;
    typedef multiset<const GraphEdge*, ElementCompare> EdgeSet;
    NodeSet nodeset;
    EdgeSet edgeset;

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
        if (!fixed || (!externalConnection && extCon) ||
            (externalConnection == extCon &&
             node->GetPosition() < fixed->GetPosition()))
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
    }

    nodeset.clear();

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
        ok = gvLayout(context, graph, "dot") == 0;
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
    m_gridSpacing = 0;
    int xspacing = WXROUND(spacing * m_dpi.x / m_dpi.y);
    if (spacing < 1) spacing = 1;
    if (xspacing < 1) xspacing = 1;
    m_diagram->SetGridSpacing(xspacing, spacing);
    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    if (canvas)
        canvas->Refresh();
}

int Graph::GetGridSpacing() const
{
    // If the spacing is only known in inches, then SetCanvas must be called
    // before the spacing can be returned in pixels as m_dpi is unknown until
    // then.
    wxCHECK(m_gridSpacing == 0, 0);
    return WXROUND(m_diagram->GetGridSpacing());
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

    Archive::Item *graph = archive.Put(tagGRAPH, tagGRAPH);
    if (!graph)
        graph = archive.Get(tagGRAPH);

    GraphCanvas *canvas = GetCanvas();
    if (canvas)
        graph->Put(tagFONT, canvas->GetFont());

    graph->Put(tagGRID, GetGridSpacing<Twips>());
    graph->Put(tagSNAP, GetSnapToGrid());
    graph->Put(tagBOUNDS, Twips::From<Pixels>(rcBounds, GetDPI()));

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
    Delete(GetElements());
    m_diagram->DeleteAllShapes();

    Archive::Item *item = archive.Get(tagGRAPH);

    if (item) {
        GraphCanvas *canvas = GetCanvas();
        wxFont font;

        if (canvas && item->Get(tagFONT, font))
            canvas->SetFont(font);

        int spacing;
        if (item->Get(tagGRID, spacing))
            SetGridSpacing<Twips>(spacing);

        bool snap;
        if (item->Get(tagSNAP, snap))
            SetSnapToGrid(snap);

        if (item->GetInstance() == NULL)
            item->SetInstance(new GraphInfo, true);
    }

    return DeserialiseInto(archive, wxPoint());
}

bool Graph::DeserialiseInto(wxInputStream& stream, const wxPoint& pt)
{
    Archive archive;
    return archive.Load(stream) && DeserialiseInto(archive, pt);
}

bool Graph::DeserialiseInto(Archive& archive, const wxPoint& pt)
{
    Archive::Item *item = archive.Get(tagGRAPH);

    if (item && item->GetInstance() == NULL) {
        GraphCanvas *canvas = GetCanvas();
        wxFont font;

        if (canvas && item->Get(tagFONT, font)) {
            wxString curdesc = canvas->GetFont().GetNativeFontInfoDesc();
            wxString newdesc = font.GetNativeFontInfoDesc();

            if (newdesc == curdesc)
                font = wxFont();
        }

        wxRect rc;
        wxPoint offset;

        if (item->Get(tagBOUNDS, rc)) {
            rc = Twips::To<Pixels>(rc, GetDPI());
            offset = pt - (rc.GetPosition() + rc.GetSize() / 2);
        }

        item->SetInstance(new GraphInfo(font, offset), true);
    }

    Archive::iterator it, end;

    for (tie(it, end) = archive.GetItems(_T(" ")); it != end; ++it) {
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

// ----------------------------------------------------------------------------
// GraphCtrl
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphCtrl, wxControl)

BEGIN_EVENT_TABLE(GraphCtrl, wxControl)
    EVT_SIZE(GraphCtrl::OnSize)
END_EVENT_TABLE()

const wxChar GraphCtrl::DefaultName[] = _T("graphctrl");

GraphCtrl::GraphCtrl(
        wxWindow *parent,
        wxWindowID winid,
        const wxPoint& pos,
        const wxSize& size,
        long style,
        const wxValidator& validator,
        const wxString& name)
  : wxControl(parent, winid, pos, size, style, validator, name),
    m_graph(NULL)
{
    GraphCanvas *canvas =
        new GraphCanvas(this, winid, wxPoint(0, 0), size, 0);
    m_canvas = canvas;
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

void GraphCtrl::SetZoom(int percent)
{
    wxPoint pt;
    {
        wxClientDC dc(m_canvas);
        m_canvas->PrepareDC(dc);
        m_canvas->GetClientSize(&pt.x, &pt.y);
        pt.x = dc.DeviceToLogicalX(pt.x / 2);
        pt.y = dc.DeviceToLogicalY(pt.y / 2);
    }

    double scale = percent / 100.0;
    m_canvas->SetScale(scale, scale);

    m_canvas->Refresh();
    m_canvas->SetCheckBounds();
    m_canvas->ScrollTo(pt, false);
}

int GraphCtrl::GetZoom()
{
    return int(m_canvas->GetScaleX() * 100.0);
}

void GraphCtrl::ScrollTo(const GraphElement& element)
{
    m_canvas->ScrollTo(element.GetPosition());
}

void GraphCtrl::EnsureVisible(const GraphElement& element)
{
    m_canvas->EnsureVisible(element.GetBounds());
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
  : m_colour(element.m_colour),
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

        if (m_shape)
            if (element.m_shape)
                SetStyle(element.GetStyle());
            else
                SetShape(NULL);

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
    Graph *graph = GetGraph();
    return graph ? graph->GetDPI() : wxSize();
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
    wxPen pen(m_colour);
    wxBrush brush(m_bgcolour);

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
  : GraphElement(colour, bgcolour, style)
{
}

GraphEdge::~GraphEdge()
{
}

void GraphEdge::SetShape(wxLineShape *line)
{
    wxLineShape *old = GetShape();
    GraphElement::SetShape(line);

    if (old && line) {
        ShowLine(this, GetFrom(), GetTo());
        old->Unlink();
    }
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
        line->AddArrow(ARROW_ARROW);

    SetShape(line);
    GraphElement::SetStyle(style);
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

    arc.Exch(_T("from"), idFrom);
    arc.Exch(_T("to"), idTo);

    if (archive.IsExtracting()) {
        GraphNode *from = archive.GetInstance<GraphNode>(idFrom);
        GraphNode *to = archive.GetInstance<GraphNode>(idTo);

        if (!ShowLine(this, from, to) || !MoveFront())
            return false;
    }

    return true;
}

GraphEdge::iterator_pair GraphEdge::GetNodes()
{
    wxLineShape *line = GetShape();

    return make_pair(iterator(new PairIterImpl(line, false)),
                     iterator(new PairIterImpl(line, true)));
}

GraphEdge::const_iterator_pair GraphEdge::GetNodes() const
{
    wxLineShape *line = GetShape();

    return make_pair(const_iterator(new PairIterImpl(line, false)),
                     const_iterator(new PairIterImpl(line, true)));
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
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);
        double x = pt.x, y = pt.y;
        canvas->Snap(&x, &y);
        shape->Erase(dc);
        shape->Move(dc, x, y, false);
        shape->Erase(dc);
        OnLayout(dc);
        GetGraph()->RefreshBounds();
    }
}

void GraphNode::SetSize(const wxSize& size)
{
    wxShape *shape = GetShape();
    wxShapeCanvas *canvas = GetCanvas(shape);

    if (canvas) {
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);
        shape->Erase(dc);
        shape->SetSize(size.x, size.y);
        shape->ResetControlPoints();
        shape->MoveLinks(dc);
        shape->Erase(dc);
        OnLayout(dc);
        GetGraph()->RefreshBounds();
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

GraphNode::iterator_pair GraphNode::GetEdges()
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();

    return make_pair(iterator(new ListIterImpl(begin)),
                     iterator(new ListIterImpl(end)));
}

GraphNode::const_iterator_pair GraphNode::GetEdges() const
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();

    return make_pair(const_iterator(new ListIterImpl(begin)),
                     const_iterator(new ListIterImpl(end)));
}

GraphNode::iterator_pair GraphNode::GetInEdges()
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();
    const int flags = ListIterImpl::InEdgesOnly;

    return make_pair(
            iterator(new ListIterImpl(begin, end, flags, false, this)),
            iterator(new ListIterImpl(end, end, flags, false, this)));
}

GraphNode::const_iterator_pair GraphNode::GetInEdges() const
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();
    const int flags = ListIterImpl::InEdgesOnly;

    return make_pair(
            const_iterator(new ListIterImpl(begin, end, flags, false, this)),
            const_iterator(new ListIterImpl(end, end, flags, false, this)));
}

GraphNode::iterator_pair GraphNode::GetOutEdges()
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();
    const int flags = ListIterImpl::OutEdgesOnly;

    return make_pair(
            iterator(new ListIterImpl(begin, end, flags, false, this)),
            iterator(new ListIterImpl(end, end, flags, false, this)));
}

GraphNode::const_iterator_pair GraphNode::GetOutEdges() const
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();
    const int flags = ListIterImpl::OutEdgesOnly;

    return make_pair(
            const_iterator(new ListIterImpl(begin, end, flags, false, this)),
            const_iterator(new ListIterImpl(end, end, flags, false, this)));
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
    arc.Exch(_T("position"), position);
    arc.Exch(_T("size"), size);

    if (arc.IsExtracting()) {
        GraphInfo *info = archive.GetInstance<GraphInfo>(tagGRAPH);
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
