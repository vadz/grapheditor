/////////////////////////////////////////////////////////////////////////////
// Name:        graphctrl.h
// Purpose:     Graph control
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef GRAPHCTRL_H
#define GRAPHCTRL_H

#include <wx/wx.h>

#include <iterator>
#include <utility>
#include <list>

/**
 * @file graphctrl.h
 * @brief Header for the graph control GUI component.
 */

class wxShape;
class wxLineShape;

namespace tt_solutions {

typedef wxShape GraphShape;
typedef wxLineShape GraphLineShape;

class Graph;
class GraphElement;
class GraphNode;

/*
 * Implementation classes
 */
namespace impl
{
    class GraphIteratorImpl;
    class GraphDiagram;
    class GraphCanvas;

    class GraphIteratorBase
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef GraphElement value_type;
        typedef ptrdiff_t difference_type;
        typedef GraphElement* pointer;
        typedef GraphElement& reference;

        GraphIteratorBase() : m_impl(NULL) { }

        GraphIteratorBase(const GraphIteratorBase& it);

        GraphIteratorBase(GraphIteratorImpl *impl) : m_impl(impl) { }

        ~GraphIteratorBase();

        GraphElement& operator*() const;

        GraphElement* operator->() const {
            return &**this;
        }

        GraphIteratorBase& operator=(const GraphIteratorBase& it);

        GraphIteratorBase& operator++();

        GraphIteratorBase operator++(int) {
            GraphIteratorBase it(*this);
            ++(*this);
            return it;
        }

        GraphIteratorBase& operator--();

        GraphIteratorBase operator--(int) {
            GraphIteratorBase it(*this);
            --(*this);
            return it;
        }

        bool operator==(const GraphIteratorBase& it) const;

        bool operator!=(const GraphIteratorBase& it) const {
            return !(*this == it);
        }

    private:
        GraphIteratorImpl *m_impl;
    };

    template <class A, class B>
    struct RefPair
    {
        RefPair(A& a, B& b) : first(a), second(b) { }

        RefPair& operator=(const std::pair<A, B>& p)
        {
            first = p.first;
            second = p.second;
            return *this;
        }

        RefPair& operator=(const RefPair<A, B>& t)
        {
            first = t.first;
            second = t.second;
            return *this;
        }

        A& first;
        B& second;
    };

} // namespace impl

/**
 * @brief Iterator class template for graph elements.
 */
template <class T>
class GraphIterator : public impl::GraphIteratorBase
{
public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T value_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef T& reference;

    GraphIterator() : Base() { }

    template <class U>
    GraphIterator(const GraphIterator<U>& it) : Base(it) {
        U *u = 0;
        CheckAssignable(u);
    }

    GraphIterator(impl::GraphIteratorImpl *impl) : Base(impl) { }

    ~GraphIterator() { }

    T& operator*() const {
        return static_cast<T&>(Base::operator*());
    }

    T* operator->() const {
        return &**this;
    }

    GraphIterator& operator=(const GraphIterator& it) {
        Base::operator=(it);
        return *this;
    }

    GraphIterator& operator++() {
        Base::operator++();
        return *this;
    }

    GraphIterator operator++(int) {
        GraphIterator it(*this);
        ++(*this);
        return it;
    }

    GraphIterator& operator--() {
        Base::operator--();
        return *this;
    }

    GraphIterator operator--(int) {
        GraphIterator it(*this);
        --(*this);
        return it;
    }

    bool operator==(const GraphIterator& it) const {
        return Base::operator==(it);
    }

    bool operator!=(const GraphIterator& it) const {
        return !(*this == it);
    }

private:
    typedef impl::GraphIteratorBase Base;

    void CheckAssignable(T*) { }
};

/**
 * @brief A helper to allow a std::pair to be assigned to two variables.
 *
 * For example:
 * @code
 *  Graph::iterator it, end;
 *
 *  for (tie(it, end) = m_graph->GetSelection(); it != end; ++it)
 *      it->SetColour(colour);
 * @endcode
 */
template <class A, class B>
impl::RefPair<A, B> tie(A& a, B& b)
{
    return impl::RefPair<A, B>(a, b);
}

/**
 * @brief An abstract base class which provides a common interface for nodes
 * and edges within a Graph object.
 *
 * @see Graph
 */
class GraphElement : public wxObject, public wxClientDataContainer
{
public:
    /** @brief Constructor. */
    GraphElement();
    /** @brief Destructor. */
    virtual ~GraphElement();

    /** @brief The element's main colour. */
    virtual wxColour GetColour() const              { return m_colour; }
    /** @brief The element's background colour. */
    virtual wxColour GetBackgroundColour() const    { return m_bgcolour; }

    /** @brief The element's main colour. */
    virtual void SetColour(const wxColour& colour);
    /** @brief The element's background colour. */
    virtual void SetBackgroundColour(const wxColour& colour);

    /**
     * @brief Selects this element.
     *
     * If the element has been added to a Graph, then
     * this adds the element to the Graph's current selection.
     */
    virtual void Select()                           { DoSelect(true); }
    /**
     * @brief Unselects this element.
     *
     * If the element has been added to a Graph, then this removes the
     * element to the Graph's current selection.
     */
    virtual void Unselect()                         { DoSelect(false); }
    /**
     * @brief Returns true if this element is selected.
     *
     * If the element has been added to a Graph, then this indicates whether
     * the element is part of the Graph's current selection.
     */
    virtual bool IsSelected() const;

    /** @brief Write a text representation of this element's attributes. */
    virtual bool Serialize(wxOutputStream& out) const = 0;
    /**
     * @brief Restore this element's attributes from text written by
     * Serialize.
     */
    virtual bool Deserialize(wxInputStream& in) = 0;

    virtual void OnDraw(wxDC& dc);

    GraphShape *GetShape() const { return DoGetShape(); }

    virtual Graph *GetGraph() const;
    virtual wxSize GetSize() const;
    virtual wxRect GetBounds() const;
    virtual void Refresh();
    virtual wxPoint GetPosition() const;

protected:
    virtual void DoSelect(bool select);
    virtual void UpdateShape() = 0;
    virtual void SetShape(GraphShape *shape);
    virtual GraphShape *DoGetShape() const { return m_shape; }

private:
    wxColour m_colour;
    wxColour m_bgcolour;

    GraphShape *m_shape;

    DECLARE_ABSTRACT_CLASS(GraphElement)
};

/**
 * @brief Represents an edge in a Graph.
 *
 * Edges are typically drawn as lines between the nodes of the graph,
 * sometimes with an arrow indicating direction.
 *
 * The Style attribute allows the selection of some predefined appearances
 * and derived classes may have additional styles.
 *
 * @see Graph
 */
class GraphEdge : public GraphElement
{
public:
    /** @brief An iterator type for returning the edge's two nodes. */
    typedef GraphIterator<GraphNode> iterator;
    /** @brief An iterator type for returning the edge's two nodes. */
    typedef GraphIterator<const GraphNode> const_iterator;
    /** @brief A begin/end pair of iterators. */
    typedef std::pair<iterator, iterator> iterator_pair;
    /** @brief A begin/end pair of iterators. */
    typedef std::pair<const_iterator, const_iterator> const_iterator_pair;

    /** @brief An enumeration of predefined appearances for edges. */
    enum Style {
        Style_Custom,
        Style_Line,
        Style_Arrow,
        Num_Styles
    };

    /** @brief Constructor. */
    GraphEdge();
    /** @brief Destructor. */
    ~GraphEdge();

    /**
     * @brief A number from the Style enumeration indicating the edge's
     * appearance.
     */
    virtual int GetStyle() const { return m_style; }

    /**
     * @brief A number from the Style enumeration indicating the edge's
     * appearance.
     */
    virtual void SetStyle(int style);

    /**
     * @brief An interator range returning the two nodes this edge connects.
     */
    iterator_pair GetNodes();
    /**
     * @brief An interator range returning the two nodes this edge connects.
     */
    const_iterator_pair GetNodes() const;

    size_t GetNodeCount() const;

    virtual GraphNode *GetFrom() const;
    virtual GraphNode *GetTo() const;

    bool Serialize(wxOutputStream& out) const;
    bool Deserialize(wxInputStream& in);

    GraphLineShape *GetShape() const;
    virtual void SetShape(GraphLineShape *shape);

protected:
    void UpdateShape() { }

private:
    int m_style;

    DECLARE_DYNAMIC_CLASS(GraphEdge)
};

/**
 * @brief Represents a node in a Graph.
 *
 * The nodes of the Graph are typically drawn as boxes or other shapes with
 * the edges drawn as lines between them.
 *
 * The Style attribute allows the selection of some predefined appearances
 * and derived classes may have additional styles.
 *
 * @see Graph
 */
class GraphNode : public GraphElement
{
public:
    /** @brief An iterator type for returning the node's list of edges. */
    typedef GraphIterator<GraphEdge> iterator;
    /** @brief An iterator type for returning the node's list of edges. */
    typedef GraphIterator<const GraphEdge> const_iterator;
    /** @brief A begin/end pair of iterators. */
    typedef std::pair<iterator, iterator> iterator_pair;
    /** @brief A begin/end pair of iterators. */
    typedef std::pair<const_iterator, const_iterator> const_iterator_pair;

    /** @brief An enumeration of predefined appearances for nodes. */
    enum Style {
        Style_Custom,
        Style_Rectangle,
        Style_Elipse,
        Style_Triangle,
        Style_Diamond,
        Num_Styles
    };

    /** @brief Constructor. */
    GraphNode();
    /** @brief Destructor. */
    ~GraphNode();

    /** @brief The node's main text label. */
    virtual wxString GetText() const        { return m_text; }
    /** @brief The node's font. */
    virtual wxFont GetFont() const;
    /**
     * @brief A number from the Style enumeration indicating the node's
     * appearance.
     */
    virtual int GetStyle() const            { return m_style; }

    /** @brief The colour of the node's text. */
    virtual wxColour GetTextColour() const  { return m_textcolour; }

    /** @brief The node's main text label. */
    virtual void SetText(const wxString& text);
    /** @brief The node's font. */
    virtual void SetFont(const wxFont& font);
    /**
     * @brief A number from the Style enumeration indicating the node's
     * appearance.
     */
    virtual void SetStyle(int style);
    /** @brief The colour of the node's text. */
    virtual void SetTextColour(const wxColour& colour);

    /**
     * @brief An interator range returning the edges connecting to this node.
     */
    iterator_pair GetEdges();
    /**
     * @brief An interator range returning the edges connecting to this node.
     */
    const_iterator_pair GetEdges() const;

    size_t GetEdgeCount() const;

    bool Serialize(wxOutputStream& out) const;
    bool Deserialize(wxInputStream& in);

    /**
     * @brief Move the node, centering it on the given point.
     */
    virtual void SetPosition(const wxPoint& pt);
    /**
     * @brief Resize the node.
     */
    virtual void SetSize(const wxSize& size);

    void SetShape(wxShape *shape);

    virtual wxPoint GetPerimeterPoint(const wxPoint& pt1,
                                      const wxPoint& pt2) const;

protected:
    virtual void DoSelect(bool select);
    virtual void UpdateShape();
    virtual void UpdateShapeTextColour();
    virtual void OnLayout(wxDC&) { }
    virtual void Layout();

private:
    int m_style;
    wxColour m_textcolour;
    wxString m_text;
    wxFont m_font;

    DECLARE_DYNAMIC_CLASS(GraphNode)
};

/**
 * @brief A control for interactive editing of a Graph object.
 *
 * The GraphCtrl is associated with a Graph object by calling SetGraph.
 * For example, you frame's OnInit() method might contain:
 *
 * @code
 *  m_graph = new Graph;
 *  m_graphctrl = new GraphCtrl(this);
 *  m_graphctrl->SetGraph(m_graph);
 *  m_graph->SetEventHandler(this);
 * @endcode
 *
 * Note that it does not take ownership of the Graph.
 *
 * In the current implementation a GraphCtrl and a Graph must be used
 * together in a one to one relationship, however in future versions it may
 * be possible for more than one GraphCtrl to update a single Graph, which
 * would allow multiple views to update a document in a doc/view
 * application.
 *
 * @see Graph
 */
class GraphCtrl : public wxControl
{
public:
    /**
     * @brief Constructor.
     */
    GraphCtrl(wxWindow *parent = NULL,
              wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxBORDER | wxRETAINED,
              const wxValidator& validator = wxDefaultValidator,
              const wxString& name = DefaultName);
    ~GraphCtrl();

    /**
     * @brief Scales the image by the given percantage.
     */
    virtual void SetZoom(int percent);
    /**
     * @brief Returns the current scaling as a percentage.
     */
    virtual int GetZoom();

    /**
     * @brief Sets the Graph object that this GraphCtrl will operate on.
     * The GraphCtrl does not take ownership.
     */
    virtual void SetGraph(Graph *graph);
    /**
     * @brief Returns the Graph object assoicated with this GraphCtrl.
     */
    virtual Graph *GetGraph() const { return m_graph; }

    /**
     * @brief Scroll the Graph so that the node is within the area visible.
     * Has no effect if the node is already visible.
     */
    virtual void EnsureVisible(const GraphElement& element);
    /** @brief Scroll the Graph, centering on the element. */
    virtual void ScrollTo(const GraphElement& element);

    /**
     * @brief Converts a point from screen coordinates to the coordinate
     * system used by the graph.
     */
    virtual wxPoint ScreenToGraph(const wxPoint& ptScreen) const;
    /**
     * @brief Converts a point from the coordinate system used by the graph
     * to screen coordinates.
     */
    virtual wxPoint GraphToScreen(const wxPoint& ptGraph) const;

    /**
     * @brief Returns the canvas window which is a child of the GraphCtrl
     * window.
     */
    virtual wxWindow *GetCanvas() const;

    void OnSize(wxSizeEvent& event);

    /**
     * @brief A default value for the constructor's name parameter.
     */
    static const wxChar DefaultName[];

private:
    impl::GraphCanvas *m_canvas;
    Graph *m_graph;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(GraphCtrl)
    DECLARE_NO_COPY_CLASS(GraphCtrl)
};

/**
 * @brief Holds a graph for editing using a GraphCtrl.
 *
 * @see GraphCtrl
 */
class Graph : public wxObject
{
public:
    /** @brief An iterator type for returning the graph's nodes and edges. */
    typedef GraphIterator<GraphElement> iterator;
    /** @brief An iterator type for returning the graph's nodes. */
    typedef GraphIterator<GraphNode> node_iterator;

    /** @brief An iterator type for returning the graph's nodes and edges. */
    typedef GraphIterator<const GraphElement> const_iterator;
    /** @brief An iterator type for returning the graph's nodes. */
    typedef GraphIterator<const GraphNode> const_node_iterator;

    /** @brief A begin/end pair of iterators returning nodes and edges. */
    typedef std::pair<iterator, iterator> iterator_pair;
    /** @brief A begin/end pair of iterators returning nodes and edges. */
    typedef std::pair<const_iterator, const_iterator> const_iterator_pair;
    /** @brief A begin/end pair of iterators returning nodes. */
    typedef std::pair<node_iterator, node_iterator> node_iterator_pair;
    /** @brief A begin/end pair of iterators returning nodes. */
    typedef std::pair<const_node_iterator, const_node_iterator> const_node_iterator_pair;

    /** @brief Constructor. */
    Graph();
    /** @brief Destructor. */
    ~Graph();

    /** @brief Adds a node to the graph. The Graph object takes ownership. */
    virtual GraphNode *Add(GraphNode *node, wxPoint pt);
    /**
     * @brief Adds an edge to the Graph, between the two nodes.
     *
     * If a GraphEdge is supplied via the edge parameter the Graph takes
     * ownership of it; if the edge parameter is omitted an edge object is
     * created implicitly.
     */
    virtual GraphEdge *Add(GraphNode& from,
                           GraphNode& to,
                           GraphEdge *edge = NULL);

    /** @brief Deletes the given node or edge. */
    virtual void Delete(GraphElement *element);
    /**
     * @brief Deletes the nodes and edges specified by the given iterator
     * range.
     */
    virtual void Delete(const iterator_pair& range);

    /** @brief Invokes a layout engine to layout the graph. */
    virtual bool LayoutAll() { return Layout(GetNodes()); }
    /**
     * @brief Invokes a layout engine to layout the subset of the graph
     * specified by the given iterator range.
     */
    virtual bool Layout(const node_iterator_pair& range);

    /**
     * @brief Adds the nodes and edges specified by the given iterator range
     * to the current selection.
     */
    virtual void Select(const iterator_pair& range);
    virtual void SelectAll() { Select(GetElements()); }

    /**
     * @brief Removes the nodes and edges specified by the given iterator
     * range from the current selection.
     */
    virtual void Unselect(const iterator_pair& range);
    virtual void UnselectAll() { Unselect(GetSelection()); }

    /** @brief An interator range returning all the nodes in the graph. */
    node_iterator_pair GetNodes();
    /**
     * @brief An interator range returning all the nodes and edges in the
     * graph.
     */
    iterator_pair GetElements();
    /**
     * @brief An interator range returning all the nodes and edges currently
     * selected.
     */
    iterator_pair GetSelection();
    /**
     * @brief An interator range returning all the nodes currently selected.
     */
    node_iterator_pair GetSelectionNodes();

    /** @brief An interator range returning all the nodes in the graph. */
    const_node_iterator_pair GetNodes() const;
    /**
     * @brief An interator range returning all the nodes and edges in the
     * graph.
     */
    const_iterator_pair GetElements() const;
    /**
     * @brief An interator range returning all the nodes and edges currently
     * selected.
     */
    const_iterator_pair GetSelection() const;
    /**
     * @brief An interator range returning all the nodes currently selected.
     */
    const_node_iterator_pair GetSelectionNodes() const;

    size_t GetNodeCount() const;
    size_t GetElementCount() const;
    size_t GetSelectionCount() const;
    size_t GetSelectionNodeCount() const;

    /**
     * @brief Write a text representation of the graph and all its elements.
     */
    virtual bool Serialize(wxOutputStream& out) const;
    /**
     * @brief Write a text representation of the graph elements specified by
     * the given iterator range.
     */
    virtual bool Serialize(wxOutputStream& out,
                           const const_iterator_pair& range) const;
    /** @brief Restore the elements from text written by Serialize. */
    virtual bool Deserialize(wxInputStream& in);

    /**
     * @brief The positions of any nodes added or moved are adjusted so that
     * they lie on a fixed spaced grid.
     */
    virtual void SetSnapToGrid(bool snap);
    /**
     * @brief The positions of any nodes added or moved are adjusted so that
     * they lie on a fixed spaced grid.
     */
    virtual bool GetSnapToGrid() const;
    /**
     * @brief The spacing of the grid used when SetSnapToGrid is switched on.
     */
    virtual void SetGridSpacing(int spacing);
    /**
     * @brief The spacing of the grid used when SetSnapToGrid is switched on.
     */
    virtual int GetGridSpacing() const;

    /** @brief Undo the last operation. */
    virtual void Undo() { wxFAIL; }
    /** @brief Redo the last Undo. */
    virtual void Redo() { wxFAIL; }

    /** @brief Indicates the previous operation could be undone with Undo. */
    virtual bool CanUndo() const { wxFAIL; return false; }
    /** @brief Indicates the previous Undo could be redone with Redo. */
    virtual bool CanRedo() const { wxFAIL; return false; }

    /** @brief Cut the current selection to the clipboard. */
    virtual bool Cut() { wxFAIL; return false; }
    /** @brief Copy the current selection to the clipboard. */
    virtual bool Copy() { wxFAIL; return false; }
    /** @brief Paste from the clipboard, replacing the current selection. */
    virtual bool Paste() { wxFAIL; return false; }
    /** @brief Delete the nodes and edges in the current selection. */
    void Clear() { Delete(GetSelection()); }

    virtual bool CanCut() const { wxFAIL; return false; }
    /** @brief Indicates that the current selection is non-empty. */
    virtual bool CanCopy() const { wxFAIL; return false; }
    /** @brief Indicates that there is graph data in the clipboard. */
    virtual bool CanPaste() const { wxFAIL; return false; }
    virtual bool CanClear() const;

    /**
     * @brief Returns a bounding rectange for the graph
     */
    wxRect GetBounds() const;
    /**
     * @brief Marks the graph bounds invalid, so that they are recalculated
     * the next time GetBounds() is called.
     */
    void RefreshBounds();

    /**
     * @brief Set an event handler to handle events from the Graph.
     */
    virtual void SetEventHandler(wxEvtHandler *handler);
    /**
     * @brief Returns the current event handler.
     */
    virtual wxEvtHandler *GetEventHandler() const;

    void SendEvent(wxEvent& event);

private:
    friend void GraphCtrl::SetGraph(Graph *graph);

    void SetCanvas(impl::GraphCanvas *canvas);
    impl::GraphCanvas *GetCanvas() const;
    void DoDelete(GraphElement *element);

    impl::GraphDiagram *m_diagram;
    mutable wxRect m_rcBounds;
    wxEvtHandler *m_handler;
    static int m_initalise;

    DECLARE_DYNAMIC_CLASS(Graph)
    DECLARE_NO_COPY_CLASS(Graph)
};

/**
 * @brief Graph event
 */
class GraphEvent : public wxNotifyEvent
{
public:
    typedef std::list<GraphNode*> NodeList;

    GraphEvent(wxEventType commandType = wxEVT_NULL, int winid = 0);
    GraphEvent(const GraphEvent& event);

    virtual wxEvent *Clone() const      { return new GraphEvent(*this); }

    void SetNode(GraphNode *node)       { m_node = node; }
    void SetTarget(GraphNode *node)     { m_target = node; }
    void SetEdge(GraphEdge *edge)       { m_edge = edge; }
    void SetPosition(const wxPoint& pt) { m_pos = pt; }
    void SetSources(NodeList& sources)  { m_sources = &sources; }

    GraphNode *GetNode() const          { return m_node; }
    GraphNode *GetTarget() const        { return m_target; }
    GraphEdge *GetEdge() const          { return m_edge; }
    wxPoint GetPosition() const         { return m_pos; }
    NodeList& GetSources() const        { return *m_sources; }

private:
    wxPoint m_pos;
    GraphNode *m_node;
    GraphNode *m_target;
    GraphEdge *m_edge;
    NodeList *m_sources;

    DECLARE_DYNAMIC_CLASS(GraphEvent)
};

typedef void (wxEvtHandler::*GraphEventFunction)(GraphEvent&);

BEGIN_DECLARE_EVENT_TYPES()

    // Graph Events

    DECLARE_EVENT_TYPE(Evt_Graph_Node_Add, wxEVT_USER_FIRST + 1101)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Delete, wxEVT_USER_FIRST + 1102)

    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Add, wxEVT_USER_FIRST + 1103)
    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Delete, wxEVT_USER_FIRST + 1104)

    DECLARE_EVENT_TYPE(Evt_Graph_Connect_Feedback, wxEVT_USER_FIRST + 1105)
    DECLARE_EVENT_TYPE(Evt_Graph_Connect, wxEVT_USER_FIRST + 1106)

    // GraphCtrl Events

    DECLARE_EVENT_TYPE(Evt_Graph_Node_Click, wxEVT_USER_FIRST + 1107)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Activate, wxEVT_USER_FIRST + 1108)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Menu, wxEVT_USER_FIRST + 1109)

    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Click, wxEVT_USER_FIRST + 1110)
    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Activate, wxEVT_USER_FIRST + 1111)
    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Menu, wxEVT_USER_FIRST + 1112)

END_DECLARE_EVENT_TYPES()

} // namespace tt_solutions

#define GraphEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction) \
        wxStaticCastEvent(tt_solutions::GraphEventFunction, &func)

#define DECLARE_GRAPH_EVT1(evt, id, fn) \
    DECLARE_EVENT_TABLE_ENTRY(tt_solutions::Evt_Graph_ ## evt, id, \
                              wxID_ANY, GraphEventHandler(fn), NULL),
#define DECLARE_GRAPH_EVT0(evt, fn) DECLARE_GRAPH_EVT1(evt, wxID_ANY, fn)

// Graph events

#define EVT_GRAPH_NODE_ADD(fn) DECLARE_GRAPH_EVT0(Node_Add, fn)
#define EVT_GRAPH_NODE_DELETE(fn) DECLARE_GRAPH_EVT0(Node_Delete, fn)

#define EVT_GRAPH_EDGE_ADD(fn) DECLARE_GRAPH_EVT0(Edge_Add, fn)
#define EVT_GRAPH_EDGE_DELETE(fn) DECLARE_GRAPH_EVT0(Edge_Delete, fn)

#define EVT_GRAPH_ELEMENT_ADD(fn) EVT_GRAPH_NODE_ADD(fn) EVT_GRAPH_EDGE_ADD(fn)
#define EVT_GRAPH_ELEMENT_DELETE(fn) EVT_GRAPH_NODE_DELETE(fn) EVT_GRAPH_EDGE_DELETE(fn)

#define EVT_GRAPH_CONNECT_FEEDBACK(fn) DECLARE_GRAPH_EVT0(Connect_Feedback, fn)
#define EVT_GRAPH_CONNECT(fn) DECLARE_GRAPH_EVT0(Connect, fn)

// GraphCtrl Events

#define EVT_GRAPH_NODE_CLICK(id, fn) DECLARE_GRAPH_EVT1(Node_Click, id, fn)
#define EVT_GRAPH_NODE_ACTIVATE(id, fn) DECLARE_GRAPH_EVT1(Node_Activate, id, fn)
#define EVT_GRAPH_NODE_MENU(id, fn) DECLARE_GRAPH_EVT1(Node_Menu, id, fn)

#define EVT_GRAPH_EDGE_CLICK(id, fn) DECLARE_GRAPH_EVT1(Edge_Click, id, fn)
#define EVT_GRAPH_EDGE_ACTIVATE(id, fn) DECLARE_GRAPH_EVT1(Edge_Activate, id, fn)
#define EVT_GRAPH_EDGE_MENU(id, fn) DECLARE_GRAPH_EVT1(Edge_Menu, id, fn)

#define EVT_GRAPH_ELEMENT_ACTIVATE(id, fn) EVT_GRAPH_NODE_ACTIVATE(id, fn) EVT_GRAPH_EDGE_ACTIVATE(id, fn)
#define EVT_GRAPH_ELEMENT_MENU(id, fn) EVT_GRAPH_NODE_MENU(id, fn) EVT_GRAPH_EDGE_MENU(id, fn)

#endif // GRAPHCTRL_H
