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
#include <wx/ogl/ogl.h>
#include <gvc.h>
#include <gd.h>

namespace tt_solutions {

using std::make_pair;
using std::pair;

// ----------------------------------------------------------------------------
// Events
// ----------------------------------------------------------------------------

DEFINE_EVENT_TYPE(Evt_Graph_Add_Node)
DEFINE_EVENT_TYPE(Evt_Graph_Add_Edge)
DEFINE_EVENT_TYPE(Evt_Graph_Adding_Edge)
DEFINE_EVENT_TYPE(Evt_Graph_Left_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Left_Double_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Right_Click)
DEFINE_EVENT_TYPE(Evt_Graph_Delete_Item)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Activated)
DEFINE_EVENT_TYPE(Evt_Graph_Node_Menu)

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

} // namespace

// ----------------------------------------------------------------------------
// GraphEvent
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(GraphEvent, wxNotifyEvent)

GraphEvent::GraphEvent(wxEventType commandType)
  : wxNotifyEvent(commandType)
{
}

GraphEvent::GraphEvent(const GraphEvent& event)
  : wxNotifyEvent(event)
{
}

// ----------------------------------------------------------------------------
// Canvas
// ----------------------------------------------------------------------------

class GraphCanvas : public wxShapeCanvas
{
public:
    GraphCanvas(wxWindow *parent = NULL, wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxBORDER | wxRETAINED,
                 const wxString& name = DefaultName)
        : wxShapeCanvas(parent, id, pos, size, style, name),
          m_graph(NULL), m_isDragging(false), m_scrolled(false)
    {
        SetScrollRate(1, 1);
    }

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

private:
    Graph *m_graph;
    bool m_isDragging;
    wxPoint m_ptDrag;
    wxPoint m_ptView;
    bool m_scrolled;

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

void GraphCanvas::OnLeftClick(double x, double y, int keys)
{
    GetGraph()->UnselectAll();
}

void GraphCanvas::OnBeginDragLeft(double, double, int keys)
{
    if (keys == 0) {
        m_isDragging = true;
        m_ptDrag = wxGetMousePosition();
        GetViewStart(&m_ptView.x, &m_ptView.y);
        CaptureMouse();
    }
}

void GraphCanvas::OnDragLeft(bool, double, double, int)
{
    if (m_isDragging) {
        wxMouseState mouse = wxGetMouseState();

        if (mouse.LeftDown()) {
            wxPoint ptDelta = m_ptDrag - wxPoint(mouse.GetX(), mouse.GetY());

            int unitX, unitY;
            GetScrollPixelsPerUnit(&unitX, &unitY);

            int x = m_ptView.x * unitX + ptDelta.x;
            int y = m_ptView.y * unitY + ptDelta.y;

            wxSize current = GetVirtualSize();
            wxSize needed = GetClientSize() + wxSize(x, y);
            wxSize size = current;

            if (needed.x > current.x)
                size.x = needed.x;
            if (needed.y > current.y)
                size.y = needed.y;

            if (size != current)
                SetVirtualSize(size.x, size.y);

            Scroll(x / unitX, y / unitY);
        }
    }
}

void GraphCanvas::OnEndDragLeft(double, double, int)
{
    if (m_isDragging) {
        ReleaseMouse();
        m_isDragging = false;
        m_scrolled = true;
        SetScrollPos(wxHORIZONTAL, GetScrollPos(wxHORIZONTAL));
        SetScrollPos(wxVERTICAL, GetScrollPos(wxVERTICAL));
    }
}

void GraphCanvas::OnScroll(wxScrollWinEvent& event)
{
    m_scrolled = true;
    event.Skip();
}

void GraphCanvas::OnIdle(wxIdleEvent& event)
{
    if (m_scrolled && !wxGetMouseState().LeftDown())
    {
        m_scrolled = false;
        wxClientDC dc(this);
        PrepareDC(dc);

        wxSize cs = GetClientSize();
        wxSize current = GetVirtualSize();
        wxSize size = current;

        int viewX, viewY;
        GetViewStart(&viewX, &viewY);
        int unitX, unitY;
        GetScrollPixelsPerUnit(&unitX, &unitY);

        int x = dc.DeviceToLogicalX(cs.x);
        int y = dc.DeviceToLogicalY(cs.y);
        wxRect b = m_graph->GetBounds();

        if (x > b.GetRight() + 1) {
            int sx = viewX * unitX + cs.x;
            if (sx < current.x)
                size.x = sx;
        }
        if (y > b.GetBottom() + 1) {
            int sy = viewY * unitY + cs.y;
            if (sy < current.y)
                size.y = sy;
        }
        if (current != size)
            SetVirtualSize(size);
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
    event.Skip();
}

// ----------------------------------------------------------------------------
// Handler to give shapes a transparent background
// ----------------------------------------------------------------------------

class GraphHandler: public wxShapeEvtHandler
{
public:
    GraphHandler(wxShapeEvtHandler *prev);

    void OnErase(wxDC& dc);
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

// ----------------------------------------------------------------------------
// Handler allow selection
// ----------------------------------------------------------------------------

class GraphElementHandler: public GraphHandler
{
public:
    GraphElementHandler(wxShapeEvtHandler *prev);
    void OnLeftClick(double x, double y, int keys, int attachment);

protected:
    void Select(wxShape *shape, bool select, int keys);
};

GraphElementHandler::GraphElementHandler(wxShapeEvtHandler *prev)
  : GraphHandler(prev)
{
}

void GraphElementHandler::OnLeftClick(double, double, int keys, int)
{
    wxShape *shape = GetShape();
    Select(shape, !shape->Selected(), keys);
}

void GraphElementHandler::Select(wxShape *shape, bool select, int keys)
{
    wxShapeCanvas *canvas = shape->GetCanvas();

    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);
    bool others = false;

    if ((keys & KEY_CTRL) == 0) {
        GraphElement *element = GetElement(shape);
        Graph *graph = element->GetGraph();
        Graph::iterator it, end;

        for (unpair(it, end) = graph->GetSelection(); it != end; ++it) {
            if (&*it != element) {
                it->GetShape()->Select(false, &dc);
                others = true;
            }
        }
    }

    shape->Select(others || select, &dc);
}

// ----------------------------------------------------------------------------
// Handler to give nodes the default behaviour
// ----------------------------------------------------------------------------

class GraphNodeHandler: public GraphElementHandler
{
public:
    GraphNodeHandler(wxShapeEvtHandler *prev);

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
    GraphNode *m_target;
    bool m_multiple;
    wxPoint m_offset;
};

GraphNodeHandler::GraphNodeHandler(wxShapeEvtHandler *prev)
  : GraphElementHandler(prev),
    m_target(NULL),
    m_multiple(false)
{
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

    m_target = NULL;
    m_offset = wxPoint(int(shape->GetX() - x), int(shape->GetY() - y));

    if (!shape->Selected())
        Select(shape, true, keys);

    Graph::node_iterator it, end;
    unpair(it, end) = graph->GetNodes(true);
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
        wxShape *target = 
            canvas->FindFirstSensitiveShape(x, y, &new_attachment, OP_ALL);

        if (target != NULL && target != shape)
            m_target = wxDynamicCast(GetElement(target), GraphNode);
        else
            m_target = NULL;
    }

    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);

    wxPen dottedPen(*wxBLACK, 1, wxDOT);
    dc.SetLogicalFunction(OGLRBLF);
    dc.SetPen(dottedPen);

    if (m_target) {
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

        for (unpair(it, end) = graph->GetNodes(true); it != end; ++it) {
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

    if (m_target) {
        Graph *graph = canvas->GetGraph();
        graph->Add(*GetNode(), *m_target);
    }
    else {
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);

        Graph *graph = canvas->GetGraph();
        double shapeX = shape->GetX();
        double shapeY = shape->GetY();
        Graph::node_iterator it, end;

        for (unpair(it, end) = graph->GetNodes(true); it != end; ++it) {
            wxShape *sh = it->GetShape();
            double xx = x - shapeX + sh->GetX() + m_offset.x;
            double yy = y - shapeY + sh->GetY() + m_offset.y;
            canvas->Snap(&xx, &yy);
            sh->Erase(dc);
            sh->Move(dc, xx, yy);
        }
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

    GetNode()->OnSize(w, h);

    x = shapeX + signX * w / 2;
    y = shapeY + signY * h / 2;
}

// ----------------------------------------------------------------------------
// GraphDiagram
// ----------------------------------------------------------------------------

class GraphDiagram : public wxDiagram
{
public:
    void AddShape(wxShape *shape, wxShape *addAfter = NULL);
    void InsertShape(wxShape *object);
};

void GraphDiagram::AddShape(wxShape *shape, wxShape *addAfter)
{
    shape->SetEventHandler(new GraphHandler(shape));
    wxDiagram::AddShape(shape, addAfter);
}

void GraphDiagram::InsertShape(wxShape *shape)
{
    shape->SetEventHandler(new GraphHandler(shape));
    wxDiagram::InsertShape(shape);
}

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

using impl::GraphIteratorImpl;

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
}

Graph::~Graph()
{
    delete m_diagram;

    if (--m_initalise == 0)
        wxOGLCleanUp();
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
    GraphEvent event(Evt_Graph_Add_Node);
    event.SetNode(node);
    event.SetPoint(pt);
    event.SetEventObject(this);
    ProcessEvent(event);

    if (event.IsAllowed()) {
        wxShape *shape = node->GetShape();
        if (!shape)
            node->SetShape(shape = DefaultShape(node));

        shape->SetEventHandler(new GraphNodeHandler(shape));
        m_diagram->wxDiagram::AddShape(shape);

        wxShapeCanvas *canvas = m_diagram->GetCanvas();
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);

        shape->Move(dc, pt.x, pt.y);
        return node;
    }

    delete node;
    return NULL;
}

GraphEdge *Graph::Add(GraphNode& from, GraphNode& to, GraphEdge *edge)
{
    if (!edge)
        edge = new GraphEdge;

    GraphEvent event(Evt_Graph_Add_Edge);
    event.SetNode(&from);
    event.SetTarget(&to);
    event.SetEdge(edge);
    event.SetEventObject(this);
    ProcessEvent(event);

    if (event.IsAllowed()) {
        wxLineShape *line = (wxLineShape*)edge->GetShape();
        if (!line)
            edge->SetShape(line = DefaultLineShape(edge));

        wxShape *fromshape = from.GetShape();
        wxShape *toshape = to.GetShape();
        fromshape->AddLine(line, toshape);

        line->SetEventHandler(new GraphElementHandler(line));
        m_diagram->wxDiagram::AddShape(line);

        wxShapeCanvas *canvas = m_diagram->GetCanvas();
        wxClientDC dc(canvas);
        canvas->PrepareDC(dc);

        fromshape->Move(dc, fromshape->GetX(), fromshape->GetY());
        toshape->Move(dc, toshape->GetX(), toshape->GetY());

        return edge;
    }

    delete edge;
    return NULL;
}

int Graph::GetGridSpacing() const
{
    return int(m_diagram->GetGridSpacing());
}

Graph::iterator_pair Graph::GetElements()
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();

    return make_pair(iterator(new ListIterImpl(begin)),
                     iterator(new ListIterImpl(end)));
}

pair<Graph::node_iterator, Graph::node_iterator>
Graph::GetNodes(bool selectionOnly)
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::NodesOnly;

    return make_pair(
        node_iterator(new ListIterImpl(begin, end, flags, selectionOnly)),
        node_iterator(new ListIterImpl(end, end, flags, selectionOnly)));
}

Graph::iterator_pair Graph::GetSelection()
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();
    const int flags = ListIterImpl::AllElements;

    return make_pair(iterator(new ListIterImpl(begin, end, flags, true)),
                     iterator(new ListIterImpl(end, end, flags, true)));
}

Graph::const_iterator_pair Graph::GetElements() const
{
    wxList *list = m_diagram->GetShapeList();
    wxList::iterator begin = list->begin(), end = list->end();

    return make_pair(const_iterator(new ListIterImpl(begin)),
                     const_iterator(new ListIterImpl(end)));
}

pair<Graph::const_node_iterator, Graph::const_node_iterator>
Graph::GetNodes() const
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

void Graph::Delete(GraphElement *element)
{
    GraphNode *node = wxDynamicCast(element, GraphNode);
    if (node)
        Delete(node);
    else
        Delete(wxStaticCast(element, GraphEdge));
}

void Graph::Delete(GraphEdge *edge)
{
    edge->GetShape()->Unlink();
    DoDelete(edge);
}

void Graph::Delete(GraphNode *node)
{
    GraphNode::iterator it, end;

    for (unpair(it, end) = node->GetEdges(); it != end; ++it)
        Delete(&*it);

    DoDelete(node);
}

void Graph::DoDelete(GraphElement *element)
{
    wxShape *shape = element->GetShape();
    shape->Select(false);
    m_diagram->RemoveShape(shape);
    delete shape;
    delete element;
}

void Graph::Delete(const iterator_pair& range)
{
    iterator i, end;
    unpair(i, end) = range;

    while (i != end)
    {
        GraphElement *element = &*i;
        ++i;
        GraphNode *node = wxDynamicCast(element, GraphNode);

        if (node) {
            GraphNode::iterator j, end_edges;

            for (unpair(j, end_edges) = node->GetEdges(); j != end_edges; ++j)
            {
                GraphEdge *edge = &*j;
                if (i != end && edge == &*i)
                    ++i;
                Delete(edge);
            }

            DoDelete(node);
        }
        else {
            Delete(wxStaticCast(element, GraphEdge));
        }
    }

    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    canvas->Refresh();
}

bool Graph::Layout(const iterator_pair& range)
{
    wxString dot;

    dot << _T("digraph Project {\n");
    dot << _T("\tnode [label=\"\", shape=box, fixedsize=true];\n");

    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);
    wxSize dpi = dc.GetPPI();
    iterator it, end;

    for (unpair(it, end) = range; it != end; ++it)
    {
        GraphEdge *edge = wxDynamicCast(&*it, GraphEdge);

        if (edge) {
            GraphEdge::iterator j, end_nodes;
            unpair(j, end_nodes) = edge->GetNodes();

            wxASSERT(j != end_nodes);
            dot << _T("\tn") << wxString::Format(_T("%p"), &*j);
            ++j;
            wxASSERT(j != end_nodes);
            dot << _T(" -> n") << wxString::Format(_T("%p"), &*j)
                << _T(";\n");
        }
        else {
            wxSize size = it->GetSize();

            dot << _T("\tn") << wxString::Format(_T("%p"), &*it)
                << _T(" [width=\"") << double(size.x) / dpi.x
                << _T("\", height=\"") << double(size.y) / dpi.y
                << _T("\"]\n");
        }
    }

    dot << _T("}\n");

    GVC_t *context = gvContext();

    Agraph_t *graph = agmemread(wx_const_cast(char*, dot.mb_str()));
    wxCHECK(graph, false);

    gvLayout(context, graph, "dot");

    double top = PS2INCH(GD_bb(graph).UR.y) * dpi.y;

    for (Agnode_t *n = agfstnode(graph); n; n = agnxtnode(graph, n))
    {
        point pos = ND_coord_i(n);
        double x = PS2INCH(pos.x) * dpi.x;
        double y = top - PS2INCH(pos.y) * dpi.y;
        GraphNode *node;
        if (sscanf(n->name, "n%p", &node) == 1)
            node->GetShape()->Move(dc, x, y, false);
    }

    gvFreeLayout(context, graph);
    agclose(graph);
    gvFreeContext(context);
    canvas->Refresh();

    return true;
}

void Graph::Select(const iterator_pair& range)
{
    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);
    iterator it, end;

    for (unpair(it, end) = range; it != end; ++it)
        it->GetShape()->Select(true, &dc);
}

void Graph::Unselect(const iterator_pair& range)
{
    wxShapeCanvas *canvas = m_diagram->GetCanvas();
    wxClientDC dc(canvas);
    canvas->PrepareDC(dc);
    iterator it, end;

    for (unpair(it, end) = range; it != end; ++it)
        it->GetShape()->Select(false, &dc);
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
    Graph *graph = new Graph;
    canvas->SetGraph(graph);
    SetGraph(graph);
}

GraphCtrl::~GraphCtrl()
{
    delete m_canvas;
    delete m_graph;
}

void GraphCtrl::SetGraph(Graph *graph)
{
    wxASSERT(graph != NULL);

    delete m_graph;
    m_graph = graph;
    m_canvas->SetDiagram(graph->m_diagram);
    graph->m_diagram->SetCanvas(m_canvas);
}

void GraphCtrl::SetZoom(int percent)
{
    double scale = percent / 100.0;
    m_canvas->SetScale(scale, scale);
    m_canvas->Refresh();
}

int GraphCtrl::GetZoom()
{
    return int(m_canvas->GetScaleX() * 100.0);
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
}

void GraphElement::Refresh()
{
    if (m_shape) {
        wxShapeCanvas *canvas = m_shape->GetCanvas();
        if (canvas) {
            wxClientDC dc(canvas);
            canvas->PrepareDC(dc);
            OnLayout(dc);
            m_shape->EraseContents(dc);
        }
    }
}

Graph *GraphElement::GetGraph() const
{
    if (m_shape) {
        wxShapeCanvas *canvas = m_shape->GetCanvas();
        if (canvas)
            return wxStaticCast(canvas, GraphCanvas)->GetGraph();
    }
    return NULL;
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
    if (m_shape) {
        wxShapeCanvas *canvas = m_shape->GetCanvas();
        if (canvas) {
            wxClientDC dc(canvas);
            canvas->PrepareDC(dc);
            m_shape->Select(select, &dc);
        }
        else {
            m_shape->Select(select);
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

void GraphElement::SetSize(const wxSize& size)
{
    wxShape *shape = GetShape();

    if (shape)
        shape->SetSize(size.x, size.y);
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

void GraphEdge::SetStyle() { }

bool GraphEdge::Serialize(wxOutputStream& out) { wxFAIL; return false; }
bool GraphEdge::Deserialize(wxInputStream& in) { wxFAIL; return false; }

pair<GraphEdge::iterator, GraphEdge::iterator>
GraphEdge::GetNodes()
{
    wxLineShape *line = GetShape();

    return make_pair(iterator(new PairIterImpl(line, false)),
                     iterator(new PairIterImpl(line, true)));
}

pair<GraphEdge::const_iterator, GraphEdge::const_iterator>
GraphEdge::GetNodes() const
{
    wxLineShape *line = GetShape();

    return make_pair(const_iterator(new PairIterImpl(line, false)),
                     const_iterator(new PairIterImpl(line, true)));
}

wxLineShape *GraphEdge::GetShape() const
{
    return static_cast<wxLineShape*>(GraphElement::GetShape());
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

void GraphNode::UpdateShape()
{
    wxShape *shape = GetShape();

    if (shape) {
        shape->AddText(m_text);
        shape->SetFont(&m_font);
        UpdateShapeTextColour();
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

pair<GraphNode::iterator, GraphNode::iterator>
GraphNode::GetEdges()
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();

    return make_pair(iterator(new ListIterImpl(begin)),
                     iterator(new ListIterImpl(end)));
}

pair<GraphNode::const_iterator, GraphNode::const_iterator>
GraphNode::GetEdges() const
{
    wxList& list = GetShape()->GetLines();
    wxList::iterator begin = list.begin(), end = list.end();

    return make_pair(const_iterator(new ListIterImpl(begin)),
                     const_iterator(new ListIterImpl(end)));
}

bool GraphNode::Serialize(wxOutputStream& out) { wxFAIL; return false; }
bool GraphNode::Deserialize(wxInputStream& in) { wxFAIL; return false; }

} // namespace tt_solutions
