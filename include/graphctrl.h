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

/*
 * Iterator class template for graph elements
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
    virtual void SetShape(GraphShape *shape);

    virtual Graph *GetGraph() const;
    virtual wxSize GetSize() const;
    virtual wxRect GetBounds() const;
    virtual void Refresh();
    virtual wxPoint GetPosition() const;

protected:
    virtual void DoSelect(bool select);
    virtual void UpdateShape() = 0;
    virtual void OnLayout(wxDC& WXUNUSED(dc)) { }
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
    enum Style { style_line, style_arrow };

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
    virtual void SetStyle();

    /**
     * @brief An interator range returning the two nodes this edge connects.
     */
    iterator_pair GetNodes();
    /**
     * @brief An interator range returning the two nodes this edge connects.
     */
    const_iterator_pair GetNodes() const;

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
    enum Style { style_rectangle };
    /**
     * @brief An enumeration for indicating what part of a node is at a given
     * point, for example the text label or image.
     */
    //enum Zone { ZONE_NONE, ZONE_ALL, ZONE_INTERIOR, ZONE_IMAGE, ZONE_TEXT };

    /** @brief Constructor. */
    GraphNode();
    /** @brief Destructor. */
    ~GraphNode();

    /** @brief The node's main text label. */
    virtual wxString GetText() const        { return m_text; }
    /** @brief The node's font. */
    virtual wxFont GetFont() const          { return m_font; }
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
    virtual void SetStyle();
    /** @brief The colour of the node's text. */
    virtual void SetTextColour(const wxColour& colour);

    /**
     * @brief Indicates what part of the node is at the given point, for
     * example the text label or image. Returns a value from the Zone
     * enumeration.
     */
    //int HitTest(const wxPoint& pt) const;

    /**
     * @brief An interator range returning the edges connecting to this node.
     */
    iterator_pair GetEdges();
    /**
     * @brief An interator range returning the edges connecting to this node.
     */
    const_iterator_pair GetEdges() const;

    bool Serialize(wxOutputStream& out) const;
    bool Deserialize(wxInputStream& in);

    virtual void OnConstrainSize(int&, int&) { }

    virtual void SetPosition(const wxPoint& pt);
    virtual void SetSize(const wxSize& size);

protected:
    void UpdateShape();
    void UpdateShapeTextColour();

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

    virtual wxPoint ScreenToGraph(const wxPoint& pt) const;

    virtual wxWindow *GetCanvas() const;

    void OnSize(wxSizeEvent& event);

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
    void RefreshBounds();

    virtual GraphShape *DefaultShape(GraphNode *node);
    virtual GraphLineShape *DefaultLineShape(GraphEdge *edge);

    virtual void SetEventHandler(wxEvtHandler *handler);
    virtual wxEvtHandler *GetEventHandler() const;

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
    GraphEvent(wxEventType commandType = wxEVT_NULL);
    GraphEvent(const GraphEvent& event);

    virtual wxEvent *Clone() const      { return new GraphEvent(*this); }

    void SetNode(GraphNode *node)       { m_node = node; }
    void SetTarget(GraphNode *node)     { m_target = node; }
    void SetEdge(GraphEdge *edge)       { m_edge = edge; }
    void SetPoint(const wxPoint& pt)    { m_point = pt; }

    GraphNode *GetNode() const          { return m_node; }
    GraphNode *GetTarget() const        { return m_target; }
    GraphEdge *GetEdge() const          { return m_edge; }
    wxPoint GetPoint() const            { return m_point; }

private:
    wxPoint m_point;
    GraphNode *m_node;
    GraphNode *m_target;
    GraphEdge *m_edge;

    DECLARE_DYNAMIC_CLASS(GraphEvent)
};

typedef void (wxEvtHandler::*GraphEventFunction)(GraphEvent&);

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(Evt_Graph_Add_Node, wxEVT_USER_FIRST + 1100)
    DECLARE_EVENT_TYPE(Evt_Graph_Add_Edge, wxEVT_USER_FIRST + 1101)
    DECLARE_EVENT_TYPE(Evt_Graph_Adding_Edge, wxEVT_USER_FIRST + 1102)
    DECLARE_EVENT_TYPE(Evt_Graph_Delete_Node, wxEVT_USER_FIRST + 1103)
    DECLARE_EVENT_TYPE(Evt_Graph_Delete_Edge, wxEVT_USER_FIRST + 1104)
    DECLARE_EVENT_TYPE(Evt_Graph_Left_Click, wxEVT_USER_FIRST + 1105)
    DECLARE_EVENT_TYPE(Evt_Graph_Left_Double_Click, wxEVT_USER_FIRST + 1106)
    DECLARE_EVENT_TYPE(Evt_Graph_Right_Click, wxEVT_USER_FIRST + 1107)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Activated, wxEVT_USER_FIRST + 1108)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Menu, wxEVT_USER_FIRST + 1109)
END_DECLARE_EVENT_TYPES()

} // namespace tt_solutions

#define GraphEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction) \
        wxStaticCastEvent(tt_solutions::GraphEventFunction, &func)

#define DECLARE_GRAPH_EVT_(evt, id, fn) \
    DECLARE_EVENT_TABLE_ENTRY(tt_solutions::Evt_Graph_ ## evt, id, \
                              wxID_ANY, GraphEventHandler(fn), NULL),

#define EVT_GRAPH_ADD_NODE(id, fn) DECLARE_GRAPH_EVT_(Add_Node, id, fn)
#define EVT_GRAPH_ADD_EDGE(id, fn) DECLARE_GRAPH_EVT_(Add_Edge, id, fn)
#define EVT_GRAPH_ADDING_EDGE(id, fn) DECLARE_GRAPH_EVT_(Adding_Edge, id, fn)
#define EVT_GRAPH_DELETE_NODE(id, fn) DECLARE_GRAPH_EVT_(Delete_Node, id, fn)
#define EVT_GRAPH_DELETE_EDGE(id, fn) DECLARE_GRAPH_EVT_(Delete_Edge, id, fn)

#define EVT_GRAPH_NODE_LEFT_CLICK(id, fn) DECLARE_GRAPH_EVT_(Node_Left_Click, id, fn)
#define EVT_GRAPH_NODE_LEFT_DOUBLE_CLICK(id, fn) DECLARE_GRAPH_EVT_(Node_Left_Double_Click, id, fn)
#define EVT_GRAPH_NODE_RIGHT_CLICK(id, fn) DECLARE_GRAPH_EVT_(Node_Right_Click, id, fn)

#define EVT_GRAPH_NODE_ACTIVATED(id, fn) DECLARE_GRAPH_EVT_(Node_Activated, id, fn)
#define EVT_GRAPH_NODE_MENU(id, fn) DECLARE_GRAPH_EVT_(Node_Menu, id, fn)

#endif // GRAPHCTRL_H
