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

#include "graphctrl.h"
#include <wx/hashset.h>
#include <wx/ogl/ogl.h>

#ifndef NO_GRAPHVIZ
#include <gvc.h>
#include <gd.h>
#endif

namespace tt_solutions {

using std::make_pair;
using std::pair;
using std::min;
using std::max;

using namespace impl;

// ----------------------------------------------------------------------------
// Events
// ----------------------------------------------------------------------------

/*DEFINE_EVENT_TYPE(Evt_Graph_Left_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Left_Double_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Right_Click)*/

// Graph Events

DEFINE_EVENT_TYPE(Evt_Graph_Node_Add)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Delete)

DEFINE_EVENT_TYPE(Evt_Graph_Edge_Add)
DEFINE_EVENT_TYPE(Evt_Graph_Edge_Adding)
DEFINE_EVENT_TYPE(Evt_Graph_Edge_Delete)

// GraphCtrl Events

DEFINE_EVENT_TYPE(Evt_Graph_Node_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Activate)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Menu)

DEFINE_EVENT_TYPE(Evt_Graph_Edge_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Edge_Activate)
DEFINE_EVENT_TYPE(Evt_Graph_Edge_Menu)

namespace {

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

WX_DECLARE_HASH_SET(GraphNode*, wxPointerHash, wxPointerEqual, NodeSet);

bool operator<(const wxPoint& pt1, const wxPoint &pt2)
{
    return pt1.y < pt2.y || (pt1.y == pt2.y && pt1.x < pt2.x);
}

wxString NodeName(const GraphNode& node)
{
    return wxString::Format(_T("n%p"), &node);
}

wxShapeCanvas *GetCanvas(wxShape *shape)
{
    return shape ? shape->GetCanvas() : NULL;
}

} // namespace

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

    void SetCheckBounds() { m_checkBounds = true; }

    void ScrollTo(const wxPoint& ptGraph, bool draw = true);
    wxPoint ScrollByOffset(int x, int y, bool draw = true);

    void EnsureVisible(const wxRect& rc);

private:
    Graph *m_graph;
    bool m_isPanning;
    bool m_checkBounds;
    wxPoint m_ptDrag;
    wxPoint m_ptOrigin;

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

void GraphCanvas::OnLeftClick(double x, double y, int keys)
{
    GetGraph()->UnselectAll();
}

void GraphCanvas::OnBeginDragLeft(double x, double y, int keys)
{
    if ((keys & KEY_SHIFT) != 0) {
        m_isPanning = true;
        m_ptDrag = wxGetMousePosition();
    }
    else {
        m_ptDrag = wxPoint(int(x), int(y));
    }
    CaptureMouse();
}

void GraphCanvas::OnDragLeft(bool, double x, double y, int)
{
    if (m_isPanning) {
        wxMouseState mouse = wxGetMouseState();

        if (mouse.LeftDown()) {
            m_ptDrag -= ScrollByOffset(m_ptDrag.x - mouse.GetX(),
                                       m_ptDrag.y - mouse.GetY());
            Update();
        }
    }
    else {
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
        m_isPanning = false;
        m_checkBounds = true;
        SetScrollPos(wxHORIZONTAL, GetScrollPos(wxHORIZONTAL));
        SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL));
    }
    else {
        Graph *graph = GetGraph();
        Graph::iterator it, end;
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

        wxClientDC dc(this);
        PrepareDC(dc);

        for (tie(it, end) = graph->GetElements(); it != end; ++it)
        {
            wxShape *shape = it->GetShape();

            if (!shape->Selected()) {
                if (rc.Intersects(it->GetBounds()))
                    shape->Select(true, &dc);
            }
            else {
                if ((key & KEY_CTRL) == 0 && !rc.Intersects(it->GetBounds()))
                    shape->Select(false, &dc);
            }
        }
    }
}

void GraphCanvas::OnScroll(wxScrollWinEvent& event)
{
    m_checkBounds = true;
    event.Skip();
}

void GraphCanvas::OnIdle(wxIdleEvent& event)
{
    if (m_checkBounds && !wxGetMouseState().LeftDown())
    {
        m_checkBounds = false;
        wxClientDC dc(this);
        PrepareDC(dc);

        wxSize cs = GetClientSize();
        wxSize current = GetVirtualSize();
        wxSize size = current;

        int viewX, viewY;
        GetViewStart(&viewX, &viewY);
        int unitX, unitY;
        GetScrollPixelsPerUnit(&unitX, &unitY);
        viewX *= unitX;
        viewY *= unitY;

        wxRect b = m_graph->GetBounds();
        b.x = dc.LogicalToDeviceX(b.x);
        b.y = dc.LogicalToDeviceY(b.y);
        b.width = dc.LogicalToDeviceXRel(b.width);
        b.height = dc.LogicalToDeviceYRel(b.height);

        int x = min(min(0, b.x) + viewX, m_ptOrigin.x);
        x -= unitX - 1;
        x -= x % unitX;
        if (x != 0) {
            m_ptOrigin.x -= x;
            size.x -= x;
            viewX -= x;
        }

        x = max(max(cs.x, b.GetRight()) + viewX, m_ptOrigin.x + cs.x);
        x += unitX - 1;
        x -= x % unitX;
        if (x != current.x)
            size.x = x;

        int y = min(min(0, b.y) + viewY, m_ptOrigin.y);
        y -= unitY - 1;
        y -= y % unitY;
        if (y != 0) {
            m_ptOrigin.y -= y;
            size.y -= y;
            viewY -= y;
        }

        y = max(max(cs.y, b.GetBottom()) + viewY, m_ptOrigin.y + cs.y);
        y += unitY - 1;
        y -= y % unitY;
        if (y != current.y)
            size.y = y;

        viewX /= unitX;
        viewY /= unitY;

#ifdef __WXMSW__
        m_xScrollPosition = viewX;
        m_yScrollPosition = viewY;
#endif

        if (current != size)
            SetVirtualSize(size);

        SetScrollPos(wxHORIZONTAL, m_xScrollPosition = viewX);
        SetScrollPos(wxVERTICAL, m_yScrollPosition = viewY);
    }
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
    Refresh();
    //SetCheckBounds();
    event.Skip();
}

void GraphCanvas::PrepareDC(wxDC& dc)
{
    dc.SetDeviceOrigin(m_ptOrigin.x, m_ptOrigin.y);
    wxShapeCanvas::PrepareDC(dc);
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
    wxSize cs = GetClientSize();
    wxSize current = GetVirtualSize();
    wxSize size = current;

    int viewX, viewY;
    GetViewStart(&viewX, &viewY);
    int unitX, unitY;
    GetScrollPixelsPerUnit(&unitX, &unitY);
    viewX *= unitX;
    viewY *= unitY;

    x -= x % unitX;
    y -= y % unitY;

    viewX += x;
    viewY += y;

    if (viewX < 0) {
        m_ptOrigin.x -= viewX;
        size.x -= viewX;
        if (draw)
            ScrollWindow(-viewX, 0);
        viewX = 0;
    }
    else if (viewX + cs.x > size.x) {
        size.x = viewX + cs.x;
    }

    if (viewY < 0) {
        m_ptOrigin.y -= viewY;
        size.y -= viewY;
        if (draw)
            ScrollWindow(0, -viewY);
        viewY = 0;
    }
    else if (viewY + cs.y > size.y) {
        size.y = viewY + cs.y;
    }

    if (size != current)
        SetVirtualSize(size);

    viewX /= unitX;
    viewY /= unitY;

    if (draw) {
        Scroll(viewX, viewY);
    }
    else {
        if (m_xScrollPosition != viewX)
            SetScrollPos(wxHORIZONTAL, m_xScrollPosition = viewX);
        if (m_yScrollPosition != viewY)
            SetScrollPos(wxVERTICAL, m_yScrollPosition = viewY);
    }

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

        wxPen *pen = shape->GetPen();
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
    if (SendEvent(cmd, x, y)) {
        wxShape *shape = GetShape();
        if (!shape->Selected())
            Select(shape, true, keys);
    }
}

void GraphElementHandler::HandleRClick(wxEventType cmd,
                                       double x, double y,
                                       int keys)
{
    wxShape *shape = GetShape();
    Select(shape, !shape->Selected(), keys);
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
    wxShapeCanvas *canvas = shape->GetCanvas();

    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);

    if ((keys & KEY_CTRL) == 0) {
        GraphElement *element = GetElement(shape);
        Graph *graph = element->GetGraph();
        Graph::iterator it, end;

        for (tie(it, end) = graph->GetSelection(); it != end; ++it) {
            if (&*it != element) {
                it->GetShape()->Select(false, &dc);
                select = true;
            }
        }
    }

    if (shape->Selected() != select)
        shape->Select(select, &dc);
}

// ----------------------------------------------------------------------------
// Handler to give nodes the default behaviour
// ----------------------------------------------------------------------------

class GraphNodeHandler: public GraphElementHandler
{
public:
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

    void CallOnSize(double& x, double& y);

    inline GraphNode *GetNode() const;

private:
    bool m_connect;
    GraphNode *m_target;
    bool m_multiple;
    wxPoint m_offset;
};

GraphNodeHandler::GraphNodeHandler(wxShapeEvtHandler *prev)
  : GraphElementHandler(prev),
    m_connect(false),
    m_target(NULL),
    m_multiple(false)
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
    Graph *graph = canvas->GetGraph();

    m_connect = false;
    m_target = NULL;
    m_offset = wxPoint(int(shape->GetX() - x), int(shape->GetY() - y));

    if (!shape->Selected())
        Select(shape, true, keys);

    Graph::node_iterator it, end;
    tie(it, end) = graph->GetSelectionNodes();
    m_multiple = it != end && ++it != end;

    OnDragLeft(true, x, y, keys, attachment);
    canvas->CaptureMouse();
}

void GraphNodeHandler::OnDragLeft(bool draw, double x, double y,
                                  int keys, int attachment)
{
    attachment = 0;

    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);

    if (!m_multiple && draw) {
        int new_attachment;
        wxShape *sh =
            canvas->FindFirstSensitiveShape(x, y, &new_attachment, OP_ALL);
        GraphNode *target;

        if (sh != NULL && sh != shape)
            target = wxDynamicCast(GetElement(sh), GraphNode);
        else
            target = NULL;

        if (target == NULL) {
            m_connect = false;
        }
        else if (target != m_target) {
            GraphEvent event(Evt_Graph_Edge_Adding);
            event.SetNode(GetNode());
            event.SetTarget(target);
            event.SetPosition(wxPoint(int(x), int(y)));
            canvas->GetGraph()->SendEvent(event);
            m_connect = event.IsAllowed();
        }

        m_target = target;
    }

    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);

    wxPen dottedPen(*wxBLACK, 1, wxDOT);
    dc.SetLogicalFunction(OGLRBLF);
    dc.SetPen(dottedPen);

    if (m_connect) {
        double xp, yp;
        shape->GetAttachmentPosition(attachment, &xp, &yp);
        dc.DrawLine(wxCoord(xp), wxCoord(yp), wxCoord(x), wxCoord(y));
    }
    else {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        Graph::node_iterator it, end;
        Graph *graph = canvas->GetGraph();
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

void GraphNodeHandler::OnEndDragLeft(double x, double y,
                                     int keys, int attachment)
{
    wxShape *shape = GetShape();
    GraphCanvas *canvas = wxStaticCast(shape->GetCanvas(), GraphCanvas);
    canvas->ReleaseMouse();

    if (m_connect) {
        Graph *graph = canvas->GetGraph();
        graph->Add(*GetNode(), *m_target);
        m_target = NULL;
        m_connect = false;
    }
    else {
        Graph *graph = canvas->GetGraph();
        wxPoint ptOffset = wxPoint(int(x), int(y)) + m_offset -
                           GetNode()->GetPosition();
        Graph::node_iterator it, end;

        for (tie(it, end) = graph->GetSelectionNodes(); it != end; ++it)
            it->SetPosition(it->GetPosition() + ptOffset);
    }
}

void GraphNodeHandler::OnDraw(wxDC& dc)
{
    GetNode()->OnDraw(dc);
}

void GraphNodeHandler::OnSizingDragLeft(wxControlPoint* pt, bool draw,
                                        double x, double y,
                                        int keys, int attachment)
{
    CallOnSize(x, y);
    GraphElementHandler::OnSizingDragLeft(pt, draw, x, y, keys, attachment);
}

void GraphNodeHandler::OnSizingEndDragLeft(wxControlPoint* pt,
                                           double x, double y,
                                           int keys, int attachment)
{
    CallOnSize(x, y);
    GraphElementHandler::OnSizingEndDragLeft(pt, x, y, keys, attachment);
    GraphNode *node = GetNode();
    node->SetSize(node->GetSize());
}

void GraphNodeHandler::CallOnSize(double& x, double& y)
{
    wxShape *shape = GetShape();

    double shapeX = shape->GetX();
    double shapeY = shape->GetY();
    int signX = x >= shapeX ? 1 : -1;
    int signY = y >= shapeY ? 1 : -1;
    int w = signX * int(x - shapeX) * 2;
    int h = signY * int(y - shapeY) * 2;

    GetNode()->OnConstrainSize(w, h);

    x = shapeX + signX * w / 2;
    y = shapeY + signY * h / 2;
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
};

void GraphDiagram::SetEventHandler(wxShape *shape)
{
    wxShapeEvtHandler *handler;

    if (shape->GetClassInfo() == CLASSINFO(wxControlPoint))
        handler = new ControlPointHandler(shape);
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

    enum Flags { AllElements, NodesOnly, EdgesOnly, Pair };
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
                 bool selectionOnly = false)
      : GraphIteratorImpl(flags, selectionOnly), m_it(begin), m_end(end)
    {
        wxASSERT(flags == AllElements ||
                 flags == NodesOnly ||
                 flags == EdgesOnly);

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
        if (GetFlags() == NodesOnly)
            return element->IsKindOf(CLASSINFO(GraphNode));
        if (GetFlags() == EdgesOnly)
            return element->IsKindOf(CLASSINFO(GraphEdge));

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

int Graph::m_initalise;

Graph::Graph()
{
    if (m_initalise++ == 0)
        wxOGLInitialize();

    m_diagram = new GraphDiagram;
    m_handler = NULL;
}

Graph::~Graph()
{
    wxASSERT_MSG(!m_diagram->GetCanvas(),
        _T("Can't delete a Graph while it is assigned to a GraphCtrl, ")
        _T("delete the GraphCtrl first or call GraphCtrl::SetGraph(NULL)"));

    Delete(GetElements());
    m_diagram->DeleteAllShapes();
    delete m_diagram;

    if (--m_initalise == 0)
        wxOGLCleanUp();
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

wxShape *Graph::DefaultShape(GraphNode *node)
{
    wxShape *shape;

    switch (node->GetStyle())
    {
        default:
            shape = new wxRectangleShape;
            break;
    }

    shape->SetSize(100, 50);
    shape->Show(true);

    return shape;
}

wxLineShape *Graph::DefaultLineShape(GraphEdge*)
{
    wxLineShape *line = new wxLineShape;
    line->MakeLineControlPoints(2);
    line->Show(true);
    line->AddArrow(ARROW_ARROW);

    return line;
}

GraphNode *Graph::Add(GraphNode *node, wxPoint pt)
{
    GraphEvent event(Evt_Graph_Node_Add);
    event.SetNode(node);
    event.SetPosition(pt);
    SendEvent(event);

    if (event.IsAllowed()) {
        wxShape *shape = node->GetShape();
        if (!shape)
            node->SetShape(shape = DefaultShape(node));

        shape->SetEventHandler(new GraphNodeHandler(shape));
        m_diagram->wxDiagram::AddShape(shape);
        node->SetPosition(pt);
        return node;
    }

    delete node;
    return NULL;
}

GraphEdge *Graph::Add(GraphNode& from, GraphNode& to, GraphEdge *edge)
{
    GraphEvent event(Evt_Graph_Edge_Add);
    event.SetNode(&from);
    event.SetTarget(&to);
    event.SetEdge(edge);
    SendEvent(event);
    edge = event.GetEdge();

    if (event.IsAllowed()) {
        if (!edge)
            edge = new GraphEdge;

        wxLineShape *line = (wxLineShape*)edge->GetShape();
        if (!line)
            edge->SetShape(line = DefaultLineShape(edge));

        wxShape *fromshape = from.GetShape();
        wxShape *toshape = to.GetShape();
        fromshape->AddLine(line, toshape);

        line->SetEventHandler(new GraphElementHandler(line));
        m_diagram->wxDiagram::AddShape(line);

        double x1, y1, x2, y2;
        line->FindLineEndPoints(&x1, &y1, &x2, &y2);
        line->SetEnds(x1, y1, x2, y2);
        edge->Refresh();

        return edge;
    }

    delete edge;
    return NULL;
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

bool Graph::Layout(const node_iterator_pair& range)
{
    wxString dot;

    dot << _T("digraph Project {\n");
    dot << _T("\tnode [label=\"\", shape=box, fixedsize=true];\n");

    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);
    wxSize dpi = dc.GetPPI();
    GraphNode *fixed = NULL;
    bool externalConnection = false;
    node_iterator i, endi;
    NodeSet nodeset;

    for (tie(i, endi) = range; i != endi; ++i)
        nodeset.insert(&*i);

    for (i = range.first; i != endi; ++i)
    {
        GraphNode *node = &*i;
        GraphNode::iterator j, endj;
        bool extCon = false;

        for (tie(j, endj) = node->GetEdges(); j != endj; ++j)
        {
            GraphEdge::iterator it = j->GetNodes().first;
            GraphNode *n1 = &*it, *n2 = &*++it;

            if (nodeset.count(n1 != node ? n1 : n2)) {
                if (n1 == node)
                    dot << _T("\t") << NodeName(*n1)
                        << _T(" -> ") << NodeName(*n2)
                        << _T(";\n");
            }
            else {
                extCon = true;
            }
        }

        if (!fixed || (!externalConnection && extCon) ||
            (externalConnection == extCon &&
             node->GetPosition() < fixed->GetPosition()))
        {
            fixed = node;
            externalConnection = extCon;
        }

        wxSize size = node->GetSize();

        dot << _T("\t") << NodeName(*node)
            << _T(" [width=\"") << double(size.x) / dpi.x
            << _T("\", height=\"") << double(size.y) / dpi.y
            << _T("\"]\n");
    }

    nodeset.clear();

    dot << _T("}\n");

#ifdef NO_GRAPHVIZ
    return false;
#else
    GVC_t *context = gvContext();

    Agraph_t *graph = agmemread(wx_const_cast(char*, dot.mb_str()));
    wxCHECK(graph, false);

    bool ok = gvLayout(context, graph, "dot") == 0;

    if (ok)
    {
        double offsetX = 0;
        double offsetY = 0;

        if (fixed) {
            Agnode_t *n = agfindnode(graph,
                           const_cast<char*>(NodeName(*fixed).mb_str()));
            if (n) {
                point pos = ND_coord_i(n);
                double x = PS2INCH(pos.x) * dpi.x;
                double y = - PS2INCH(pos.y) * dpi.y;
                wxPoint pt = fixed->GetPosition();
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
                node->SetPosition(wxPoint(x, y));
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
    gvFreeContext(context);
    return ok;
#endif
}

void Graph::Select(const iterator_pair& range)
{
    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);
    iterator it, end;

    for (tie(it, end) = range; it != end; ++it)
        if (!it->GetShape()->Selected())
            it->GetShape()->Select(true, &dc);
}

void Graph::Unselect(const iterator_pair& range)
{
    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);
    iterator it, end;

    for (tie(it, end) = range; it != end; ++it)
        if (it->GetShape()->Selected())
            it->GetShape()->Select(false, &dc);
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
    m_diagram->SetGridSpacing(spacing);
}

int Graph::GetGridSpacing() const
{
    return int(m_diagram->GetGridSpacing());
}

bool Graph::CanClear() const
{
    const_iterator_pair its = GetSelection();
    return its.first == its.second;
}

bool Graph::Serialize(wxOutputStream& out) const
{
    wxFAIL;
    return false;
}

bool Graph::Serialize(wxOutputStream& out,
                      const const_iterator_pair& range) const
{
    wxFAIL;
    return false;
}

bool Graph::Deserialize(wxInputStream& in)
{
    wxFAIL;
    return false;
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
    wxClientDC dc(m_canvas);
    m_canvas->PrepareDC(dc);
    wxPoint pt = m_canvas->ScreenToClient(ptScreen);
    pt.x = dc.DeviceToLogicalX(pt.x);
    pt.y = dc.DeviceToLogicalY(pt.y);
    return pt;
}

wxPoint GraphCtrl::GraphToScreen(const wxPoint& ptGraph) const
{
    wxClientDC dc(m_canvas);
    m_canvas->PrepareDC(dc);
    wxPoint pt(dc.LogicalToDeviceX(ptGraph.x),
               dc.LogicalToDeviceY(ptGraph.y));
    return m_canvas->ClientToScreen(pt);
}

wxWindow *GraphCtrl::GetCanvas() const
{
    return m_canvas;
}

void GraphCtrl::OnSize(wxSizeEvent& event)
{
    m_canvas->SetSize(GetClientSize());
}

// ----------------------------------------------------------------------------
// GraphElement
// ----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(GraphElement, wxObject)

GraphElement::GraphElement()
  : m_colour(*wxBLACK),
    m_bgcolour(*wxWHITE),
    m_shape(NULL)
{
}

GraphElement::~GraphElement()
{
    delete m_shape;
}

Graph *GraphElement::GetGraph() const
{
    wxShapeCanvas *canvas = GetCanvas(m_shape);
    return canvas ? wxStaticCast(canvas, GraphCanvas)->GetGraph() : NULL;
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
        m_shape->Select(select);
        wxShapeCanvas *canvas = GetCanvas(m_shape);

        if (canvas) {
            wxClientDC dc(canvas);
            canvas->PrepareDC(dc);
            m_shape->OnEraseControlPoints(dc);
        }
    }
}

bool GraphElement::IsSelected() const
{
    return m_shape && m_shape->Selected();
}

void GraphElement::SetShape(wxShape *shape)
{
    wxASSERT(m_shape == NULL);
    m_shape = shape;
    shape->SetClientData(this);
    UpdateShape();
    Refresh();
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
    wxPoint pt(0, 0);

    if (shape) {
        pt.x = WXROUND(shape->GetX());
        pt.y = WXROUND(shape->GetY());
    }

    return pt;
}

wxSize GraphElement::GetSize() const
{
    wxShape *shape = GetShape();
    wxSize size(0, 0);

    if (shape) {
        double width, height;
        shape->GetBoundingBoxMin(&width, &height);
        size.x = WXROUND(width);
        size.y = WXROUND(height);
    }

    return size;
}

// ----------------------------------------------------------------------------
// GraphEdge
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphEdge, GraphElement)

GraphEdge::GraphEdge()
{
}

GraphEdge::~GraphEdge()
{
}

void GraphEdge::SetStyle()
{
}

bool GraphEdge::Serialize(wxOutputStream& out) const
{
    wxFAIL;
    return false;
}

bool GraphEdge::Deserialize(wxInputStream& in)
{
    wxFAIL;
    return false;
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

wxLineShape *GraphEdge::GetShape() const
{
    return static_cast<wxLineShape*>(GraphElement::DoGetShape());
}

void GraphEdge::SetShape(wxLineShape *line)
{
    GraphElement::SetShape(line);
}

// ----------------------------------------------------------------------------
// GraphNode
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphNode, GraphElement)

GraphNode::GraphNode()
  : m_style(0),
    m_textcolour(*wxBLACK)
{
}

GraphNode::~GraphNode()
{
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
        double x = pt.x, y = pt.y;
        canvas->Snap(&x, &y);
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);
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
        Refresh();
    }
}

void GraphNode::SetFont(const wxFont& font)
{
    m_font = font;

    wxShape *shape = GetShape();

    if (shape) {
        shape->SetFont(&m_font);
        Refresh();
    }
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
            name << _T("RGB-") << m_textcolour.Red() << "-"
                 << m_textcolour.Green() << "-" << m_textcolour.Blue();
            wxTheColourDatabase->AddColour(name, m_textcolour);
        }
        shape->SetTextColour(name);
    }
}

void GraphNode::SetStyle() { wxFAIL; }

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

bool GraphNode::Serialize(wxOutputStream& out) const
{
    wxFAIL;
    return false;
}

bool GraphNode::Deserialize(wxInputStream& in)
{
    wxFAIL;
    return false;
}

} // namespace tt_solutions
