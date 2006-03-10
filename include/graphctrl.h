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

#include "wx/wx.h"

/**
 * @file graphctrl.h
 * @brief Header for the graph control GUI component.
 */

namespace tt_solutions {

class Graph;
class GraphElement;
class GraphNode;

/*
 * Implementation classes
 */
namespace implementation
{
    class GraphDiagram;
    class GraphCanvas;
    class GraphNodeImpl;
    class GraphEdgeImpl;
    class GraphIteratorImpl;

    class GraphIteratorBase
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef GraphElement value_type;
        typedef ptrdiff_t difference_type;
        typedef GraphElement* pointer;
        typedef GraphElement& reference;

        GraphIteratorBase();

        GraphIteratorBase(const GraphIteratorBase& it);

        ~GraphIteratorBase();

        const GraphElement& operator *() const;

        const GraphElement* operator ->() const {
            return &**this;
        }

        GraphIteratorBase& operator =(const GraphIteratorBase& it);

        GraphIteratorBase& operator ++();

        GraphIteratorBase operator ++(int) {
            GraphIteratorBase it(*this);
            ++(*this);
            return it;
        }

        GraphIteratorBase& operator --();

        GraphIteratorBase operator --(int) {
            GraphIteratorBase it(*this);
            --(*this);
            return it;
        }

        bool operator ==(const GraphIteratorBase& it) const;

        bool operator !=(const GraphIteratorBase& it) const {
            return !(*this == it);
        }

    private:
        GraphIteratorImpl *m_impl;
    };
} // namespace implementation

/*
 * Iterator class template for graph elements
 */
template <class T>
class GraphIterator : private implementation::GraphIteratorBase
{
public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T value_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef T& reference;

    GraphIterator() { }

    GraphIterator(const GraphIterator& it) : B(it) { }

    template <class U> GraphIterator(const GraphIterator<U>& it) : B(it) { }

    ~GraphIterator() { }

    const T& operator *() const {
        return static_cast<T&>(B::operator*());
    }

    const T* operator ->() const {
        return &**this;
    }

    GraphIterator& operator =(const GraphIterator& it) {
        B::operator=(it);
        return *this;
    }

    GraphIterator& operator ++() {
        B::operator++();
        return *this;
    }

    GraphIterator operator ++(int) {
        GraphIterator it(*this);
        ++(*this);
        return it;
    }

    GraphIterator& operator --() {
        B::operator--();
        return *this;
    }

    GraphIterator operator --(int) {
        GraphIterator it(*this);
        --(*this);
        return it;
    }

    bool operator ==(const GraphIterator& it) const {
        return B::operator==(it);
    }

    bool operator !=(const GraphIterator& it) const {
        return !(*this == it);
    }

private:
    typedef implementation::GraphIteratorBase B;
};

/**
 * @brief An abstract base class which provides a common interface for nodes
 * and edges within a Graph object.
 *
 * @see Graph
 */
class GraphElement : public wxObject
{
public:
    /** @brief Constructor. */
    GraphElement() { }
    /** @brief Destructor. */
    virtual ~GraphElement() { }

    /** @brief The element's main colour. */
    virtual wxColour GetColour() const = 0;
    /** @brief The element's background colour. */
    virtual wxColour GetBackgroundColour() const = 0;

    /** @brief The element's main colour. */
    virtual void SetColour(const wxColour& colour) = 0;
    /** @brief The element's background colour. */
    virtual void SetBackgroundColour(const wxColour& colour) = 0;

    /**
     * @brief Selects this element.
     *
     * If the element has been added to a Graph, then
     * this adds the element to the Graph's current selection.
     */
    virtual void Select() = 0;
    /**
     * @brief Unselects this element.
     *
     * If the element has been added to a Graph, then this removes the
     * element to the Graph's current selection.
     */
    virtual void Unselect() = 0;
    /**
     * @brief Returns true if this element is selected.
     *
     * If the element has been added to a Graph, then this indicates whether
     * the element is part of the Graph's current selection.
     */
    virtual bool IsSelected() = 0;

    /** @brief Write a text representation of this element's attributes. */
    virtual bool Serialize(wxOutputStream& out) = 0;
    /**
     * @brief Restore this element's attributes from text written by
     * Serialize.
     */
    virtual bool Deserialize(wxInputStream& in) = 0;
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
    int GetStyle() const                 { return m_style; }
    wxColour GetColour() const           { return m_colour; }
    wxColour GetBackgroundColour() const { return m_bgcolour; }
    bool IsSelected()                    { return m_selected; }

    /**
     * @brief A number from the Style enumeration indicating the edge's
     * appearance.
     */
    void SetStyle();
    void SetColour(const wxColour& colour);
    void SetBackgroundColour(const wxColour& colour);
    void Select();
    void Unselect();

    /**
     * @brief An interator range returning the two nodes this edge connects.
     */
    std::pair<iterator, iterator> GetNodes();
    /**
     * @brief An interator range returning the two nodes this edge connects.
     */
    std::pair<const_iterator, const_iterator> GetNodes() const;

    bool Serialize(wxOutputStream& out);
    bool Deserialize(wxInputStream& in);

private:
    int m_style;
    wxColour m_colour;
    wxColour m_bgcolour;
    bool m_selected;

    implementation::GraphEdgeImpl *m_handler;
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

    /** @brief An enumeration of predefined appearances for nodes. */
    enum Style { style_rectangle };
    /**
     * @brief An enumeration for indicating what part of a node is at a given
     * point, for example the text label or image.
     */
    enum HitValue { hittest_none, hittest_image, hittest_text };

    /** @brief Constructor. */
    GraphNode();
    /** @brief Destructor. */
    ~GraphNode();

    /** @brief The node's main text label. */
    wxString GetText() const             { return m_text; }
    /** @brief The node's font. */
    wxFont GetFont() const               { return m_font; }
    /**
     * @brief A number from the Style enumeration indicating the node's
     * appearance.
     */
    int GetStyle() const                 { return m_style; }
    wxColour GetColour() const           { return m_colour; }
    /** @brief The colour of the node's text. */
    wxColour GetTextColour() const       { return m_textcolour; }
    wxColour GetBackgroundColour() const { return m_bgcolour; }
    bool IsSelected()                    { return m_selected; }

    /** @brief The node's main text label. */
    void SetText(const wxString& text);
    /** @brief The node's font. */
    void SetFont(const wxFont& font);
    /**
     * @brief A number from the Style enumeration indicating the node's
     * appearance.
     */
    void SetStyle();
    void SetColour(const wxColour& colour);
    /** @brief The colour of the node's text. */
    void SetTextColor(const wxColour& colour);
    void SetBackgroundColour(const wxColour& colour);
    void Select();
    void Unselect();

    /**
     * @brief Indicates what part of the node is at the given point, for example
     * the text label or image. Returns a value from the HitValue
     * enumeration.
     */
    int HitTest(const wxPoint& pt) const;

    /**
     * @brief An interator range returning the edges connecting to this node.
     */
    std::pair<iterator, iterator> GetEdges();
    /**
     * @brief An interator range returning the edges connecting to this node.
     */
    std::pair<const_iterator, const_iterator> GetEdges() const;

    bool Serialize(wxOutputStream& out);
    bool Deserialize(wxInputStream& in);

private:
    int m_style;
    wxColour m_colour;
    wxColour m_textcolour;
    wxColour m_bgcolour;
    bool m_selected;
    wxString m_text;
    wxFont m_font;
    implementation::GraphNodeImpl *m_handler;
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
class GraphCtrl : public wxScrolledWindow
{
public:
    /**
     * @brief Constructor.
     */
    GraphCtrl(wxWindow *parent,
              wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxBORDER | wxRETAINED,
              const wxString& name = DefaultName);
    ~GraphCtrl();

    /**
     * @brief Scales the image by the given percantage.
     */
    void SetZoom(int percent);
    /**
     * @brief Returns the current scaling as a percentage.
     */
    int GetZoom();

    /**
     * @brief Sets the Graph object that this GraphCtrl will operate on.
     * The GraphCtrl does not take ownership.
     */
    void SetGraph(Graph *graph);
    /**
     * @brief Returns the Graph object assoicated with this GraphCtrl.
     */
    Graph *GetGraph() const { return m_graph; }

    /**
     * @brief Scroll the Graph so that the node is within the area visible.
     * Has no effect if the node is already visible.
     */
    void EnsureVisible(const GraphNode& node);
    /** @brief Scroll the Graph, centering on the node. */
    void ScrollTo(const GraphNode& node);

    void OnSize(wxSizeEvent& event);

    static const wxChar DefaultName[];

private:
    implementation::GraphCanvas *m_canvas;
    Graph *m_graph;

    DECLARE_EVENT_TABLE()
};

/**
 * @brief Holds a graph for editing using a GraphCtrl.
 *
 * @see GraphCtrl
 */
class Graph : public wxEvtHandler
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

    /** @brief Constructor. */
    Graph();
    /** @brief Destructor. */
    ~Graph();

    /** @brief Adds a node to the graph. The Graph object takes ownership. */
    GraphNode *Add(GraphNode *node, wxPoint pt = wxDefaultPosition);
    /**
     * @brief Adds an edge to the Graph, between the two nodes.
     *
     * If a GraphEdge is supplied via the edge parameter the Graph takes
     * ownership of it; if the edge parameter is omitted an edge object is
     * created implicitly.
     */
    GraphEdge *Add(const GraphNode& from, const GraphNode& to, GraphEdge *edge = NULL);

    /** @brief Deletes the given node or edge. */
    void Delete(GraphElement *element);
    /**
     * @brief Deletes the nodes and edges specified by the given iterator
     * range.
     */
    void Delete(const iterator_pair& range);

    /** @brief Invokes a layout engine to layout the graph. */
    bool Layout();
    /**
     * @brief Invokes a layout engine to layout the subset of the graph
     * specified by the given iterator range.
     */
    bool Layout(const iterator_pair& range);

    /** @brief Adds the given node or edge to the current selection. */
    void Select(GraphElement *element);
    /**
     * @brief Adds the nodes and edges specified by the given iterator range
     * to the current selection.
     */
    void Select(const iterator_pair& range);

    /** @brief Removes the given node or edge from the current selection. */
    void Unselect(GraphElement *element);
    /**
     * @brief Removes the nodes and edges specified by the given iterator
     * range from the current selection.
     */
    void Unselect(const iterator_pair& range);

    /** @brief An interator range returning all the nodes in the graph. */
    std::pair<node_iterator, node_iterator> GetNodes();
    /**
     * @brief An interator range returning all the nodes and edges in the
     * graph.
     */
    iterator_pair GetElements();
    /**
     * @brief An interator range returning all the nodes and edges currently
     * selected.
     */
    iterator_pair GetSelected();

    /** @brief An interator range returning all the nodes in the graph. */
    std::pair<const_node_iterator, const_node_iterator> GetNodes() const;
    /**
     * @brief An interator range returning all the nodes and edges in the
     * graph.
     */
    const_iterator_pair GetElements() const;
    /**
     * @brief An interator range returning all the nodes and edges currently
     * selected.
     */
    const_iterator_pair GetSelected() const;

    /**
     * @brief Write a text representation of the graph and all its elements.
     */
    bool Serialize(wxOutputStream& out) const;
    /**
     * @brief Write a text representation of the graph elements specified by
     * the given iterator range.
     */
    bool Serialize(wxOutputStream& out, const_iterator_pair range) const;
    /** @brief Restore the elements from text written by Serialize. */
    bool Deserialize(wxInputStream& in);

    /**
     * @brief The positions of any nodes added or moved are adjusted so that
     * they lie on a fixed spaced grid.
     */
    void SetSnapToGrid(bool snap);
    /**
     * @brief The positions of any nodes added or moved are adjusted so that
     * they lie on a fixed spaced grid.
     */
    bool GetSnapToGrid() const;
    /**
     * @brief The spacing of the grid used when SetSnapToGrid is switched on.
     */
    void SetGridSpacing(int spacing);
    /**
     * @brief The spacing of the grid used when SetSnapToGrid is switched on.
     */
    int GetGridSpacing() const;

    /** @brief Undo the last operation. */
    void Undo();
    /** @brief Redo the last Undo. */
    void Redo();

    /** @brief Indicates the previous operation could be undone with Undo. */
    bool CanUndo() const;
    /** @brief Indicates the previous Undo could be redone with Redo. */
    bool CanRedo() const;

    /** @brief Cut the current selection to the clipboard. */
    bool Cut();
    /** @brief Copy the current selection to the clipboard. */
    bool Copy();
    /** @brief Paste from the clipboard, replacing the current selection. */
    bool Paste();
    /** @brief Delete the nodes and edges in the current selection. */
    void Clear();

    bool CanCut() const;
    /** @brief Indicates that the current selection is non-empty. */
    bool CanCopy() const;
    /** @brief Indicates that there is graph data in the clipboard. */
    bool CanPaste() const;
    bool CanClear() const;

private:
    friend void GraphCtrl::SetGraph(Graph *graph);

    implementation::GraphDiagram *m_diagram;

    DECLARE_EVENT_TABLE()
};

/**
 * @brief Graph event
 */
class GraphEvent : public wxNotifyEvent
{
public:
    GraphEvent(wxEventType commandType = wxEVT_NULL, int id = 0);
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

} // namespace tt_solutions

#ifndef wxEVT_GRAPH_FIRST
#   define wxEVT_GRAPH_FIRST wxEVT_USER_FIRST + 1100
#endif

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_ADD_NODE, wxEVT_GRAPH_FIRST)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_ADD_EDGE, wxEVT_GRAPH_FIRST + 1)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_ADDING_EDGE, wxEVT_GRAPH_FIRST + 2)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_LEFT_CLICK, wxEVT_GRAPH_FIRST + 3)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_LEFT_DOUBLE_CLICK, wxEVT_GRAPH_FIRST + 4)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_RIGHT_CLICK, wxEVT_GRAPH_FIRST + 5)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_DELETE_ITEM, wxEVT_GRAPH_FIRST + 6)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_NODE_ACTIVATED, wxEVT_GRAPH_FIRST + 7)
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_GRAPH_NODE_MENU, wxEVT_GRAPH_FIRST + 8)
END_DECLARE_EVENT_TYPES()

#define GraphEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(GraphEventFunction, &func)

#define wx__DECLARE_GRAPHEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_COMMAND_GRAPH_ ## evt, id, GraphEventHandler(fn))

#define EVT_GRAPH_ADD_NODE(id, fn) wx__DECLARE_GRAPHEVT(ADD_NODE, id, fn)
#define EVT_GRAPH_ADD_EDGE(id, fn) wx__DECLARE_GRAPHEVT(ADD_EDGE, id, fn)
#define EVT_GRAPH_ADDING_EDGE(id, fn) wx__DECLARE_GRAPHEVT(ADDING_EDGE, id, fn)

#define EVT_GRAPH_NODE_LEFT_CLICK(id, fn) wx__DECLARE_GRAPHEVT(NODE_LEFT_CLICK, id, fn)
#define EVT_GRAPH_NODE_LEFT_DOUBLE_CLICK(id, fn) wx__DECLARE_GRAPHEVT(NODE_LEFT_DOUBLE_CLICK, id, fn)
#define EVT_GRAPH_NODE_RIGHT_CLICK(id, fn) wx__DECLARE_GRAPHEVT(NODE_RIGHT_CLICK, id, fn)

#define EVT_GRAPH_DELETE_ITEM(id, fn) wx__DECLARE_GRAPHEVT(DELETE_ITEM, id, fn)

#define EVT_GRAPH_NODE_ACTIVATED(id, fn) wx__DECLARE_GRAPHEVT(NODE_ACTIVATED, id, fn)
#define EVT_GRAPH_NODE_MENU(id, fn) wx__DECLARE_GRAPHEVT(NODE_MENU, id, fn)

#endif // GRAPHCTRL_H
