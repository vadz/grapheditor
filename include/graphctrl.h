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
#include <list>

#include "factory.h"
#include "archive.h"
#include "coords.h"
#include "tie.h"

/**
 * @file graphctrl.h
 * @brief Header for the graph control GUI component.
 */

class wxShape;
class wxLineShape;
class wxXmlNode;
class wxXmlProperty;

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

    class Initialisor
    {
    public:
        Initialisor();
        ~Initialisor();

    private:
        Initialisor(const Initialisor&) { }
        static int m_initalise;
    };

} // namespace impl

/**
 * @brief Iterator class template for graph elements.
 *
 * Graph elements are enumerated using iterator types such as
 * <code>Graph::iterator</code>, <code>GraphNode::iterator</code> and
 * <code>GraphEdge::iterator</code>, which are instantiations of this
 * template, <code>GraphIterator<T></code>.
 *
 * A <code>GraphIterator<GraphNode></code> or
 * <code>GraphIterator<GraphEdge></code> is assignable to a
 * <code>GraphIterator<GraphElement></code>, but not vice versa.
 *
 * Methods that return iterators return a begin/end pair in a
 * <code>std::pair</code>.  These can be assigned to a pair of variables
 * using the '<code>tie</code>' function, so the usual idiom for using them
 * is:
 *
 * @code
 *  Graph::node_iterator it, end;
 *
 *  for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it)
 *      it->SetSize(size);
 * @endcode
 *
 * As with <code>std::list</code>, deleting an element from a graph
 * invalidates any iterators pointing to that element, but not those pointing
 * to any other elements. Therefore when deleting elements in a loop, it is
 * necessary to increment the loop iterator before deleting the element that
 * it points to.
 *
 * Also affected in the same way are the <code>Select()</code>,
 * <code>SetStyle()</code> and <code>SetShape()</code> methods of nodes and
 * edges. These also invalidate any iterators pointing to the elements they
 * change. For example:
 *
 * @code
 *  iterator i, j, end;
 *  tie(i, end) = range;
 *
 *  while (i != end) {
 *      // the loop iterator i, must be incremented before Select is called
 *      j = i++;
 *      if (j->GetEdgeCount() == 0)
 *          j->Select();
 *  }
 * @endcode
 *
 * @see
 * tie()
 * Graph::GetNodes()
 * Graph::GetSelection()
 * GraphNode::GetEdges()
 * GraphEdge::GetNodes()
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
 * @brief An abstract base class which provides a common interface for nodes
 * and edges within a Graph object.
 *
 * @see Graph
 */
class GraphElement : public wxObject, public wxClientDataContainer
{
public:
    /** @brief An enumeration of predefined appearances for elements. */
    enum Style {
        Style_Custom
    };

    /** @brief Constructor. */
    GraphElement(const wxColour& colour,
                 const wxColour& bgcolour,
                 int style);
    /** @brief Destructor. */
    virtual ~GraphElement();

    /** @brief Copy constructor. */
    GraphElement(const GraphElement& element);
    /** @brief Assignment operator. */
    GraphElement& operator=(const GraphElement& element);

    /**
     * @brief A number from the Style enumeration indicating the element's
     * appearance.
     */
    virtual int GetStyle() const { return m_style; }

    /**
     * @brief A number from the Style enumeration indicating the element's
     * appearance.
     *
     * Invalidates any iterators pointing to this element.
     */
    virtual void SetStyle(int style) { m_style = style; }

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
     *
     * Invalidates any iterators pointing to this element.
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

    /**
     * @brief Write or restore this element's attributes to an archive.
     */
    virtual bool Serialise(Archive::Item& arc);

    /**
     * @brief Called by the graph control when the element must draw itself.
     * Can be overridden to give the element a custom appearance.
     */
    virtual void OnDraw(wxDC& dc);

    /**
     * @brief Returns the shape that represents this graph element in the
     * underlying graphics library.
     */
    GraphShape *GetShape() const { return DoGetShape(); }
    GraphShape *EnsureShape() { return DoEnsureShape(); }

    /**
     * @brief Returns the graph that this element has been added to, or
     * NULL if it has not be added.
     */
    virtual Graph *GetGraph() const;

    /**
     * @brief Returns the size of the graph element in graph coordinates.
     */
    virtual wxSize GetSize() const;
    template <class T> wxSize GetSize() const;
    /**
     * @brief Returns the position of the graph element in graph coordinates.
     */
    virtual wxPoint GetPosition() const;
    template <class T> wxPoint GetPosition() const;
    /**
     * @brief Returns the bounding rectangle of the graph element in graph
     * coordinates.
     */
    virtual wxRect GetBounds() const;

    /**
     * @brief Invalidates the bounds of the element so that it redraws the
     * next time its graph control handles a wxEVT_PAINT event.
     */
    virtual void Refresh();

    virtual void Layout() = 0;

protected:
    virtual void DoSelect(bool select);
    virtual void UpdateShape() = 0;
    virtual void SetShape(GraphShape *shape);
    virtual GraphShape *DoGetShape() const { return m_shape; }
    virtual GraphShape *DoEnsureShape();
    virtual wxSize GetDPI() const;

private:
    impl::Initialisor m_initalise;

    wxColour m_colour;
    wxColour m_bgcolour;

    int m_style;
    GraphShape *m_shape;

    DECLARE_ABSTRACT_CLASS(GraphElement)
};

// Inline defintions

template <class T> wxSize GraphElement::GetSize() const
{
    return Pixels::To<T>(GetSize(), GetDPI());
}

template <class T> wxPoint GraphElement::GetPosition() const
{
    return Pixels::To<T>(GetPosition(), GetDPI());
}

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
        Style_Custom = GraphElement::Style_Custom,
        Style_Line,
        Style_Arrow,
        Num_Styles
    };

    /** @brief Constructor. */
    GraphEdge(const wxColour& colour = *wxBLACK,
              const wxColour& bgcolour = *wxWHITE,
              int style = Style_Arrow);
    /** @brief Destructor. */
    ~GraphEdge();

    /**
     * @brief A number from the Style enumeration indicating the edge's
     * appearance.
     *
     * Invalidates any iterators pointing to this element.
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
    /**
     * @brief Returns the number of nodes this edge connects, i.e. two if the
     * edge has been added to a graph.
     */
    size_t GetNodeCount() const;

    /**
     * @brief Returns the first of the two nodes this edge connects.
     */
    virtual GraphNode *GetFrom() const;
    /**
     * @brief Returns the second of the two nodes this edge connects.
     */
    virtual GraphNode *GetTo() const;

    bool Serialise(Archive::Item& arc);

    GraphLineShape *GetShape() const;
    GraphLineShape *EnsureShape();

    /**
     * @brief Set a shape object from the underlying graphics library that
     * will be used to render this edge on the graph control.
     *
     * This makes user code dependent on the particular underlying graphics
     * library. To avoid a dependency, <code>SetStyle()</code> can be used
     * instead to select from a limit range of prefined appearances. Or for
     * more control <code>OnDraw()</code> can be overridden.
     *
     * Invalidates any iterators pointing to this element.
     */
    virtual void SetShape(GraphLineShape *shape);

    virtual void Layout() { }

protected:
    void UpdateShape() { }
    bool MoveFront();

private:
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
        Style_Custom = GraphElement::Style_Custom,
        Style_Rectangle,
        Style_Elipse,
        Style_Triangle,
        Style_Diamond,
        Num_Styles
    };

    /** @brief Constructor. */
    GraphNode(const wxString& text = wxEmptyString,
              const wxColour& colour = *wxBLACK,
              const wxColour& bgcolour = *wxWHITE,
              const wxColour& textcolour = *wxBLACK,
              int style = Style_Rectangle);
    /** @brief Destructor. */
    ~GraphNode();

    /** @brief The node's main text label. */
    virtual wxString GetText() const        { return m_text; }
    /** @brief The node's font. */
    virtual wxFont GetFont() const;

    /** @brief The colour of the node's text. */
    virtual wxColour GetTextColour() const  { return m_textcolour; }

    /** @brief The node's main text label. */
    virtual void SetText(const wxString& text);
    /**
     * @brief The node's font.
     *
     * If no font is set for the node it inherits the font of the graph
     * control.
     */
    virtual void SetFont(const wxFont& font);
    /**
     * @brief A number from the Style enumeration indicating the node's
     * appearance.
     *
     * Invalidates any iterators pointing to this element.
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
    /**
     * @brief Returns the number of edges connecting to this node.
     */
    size_t GetEdgeCount() const;

    /**
     * @brief An iterator range returning the edges into this node.
     */
    iterator_pair GetInEdges();
    /**
     * @brief An iterator range returning the edges into this node.
     */
    const_iterator_pair GetInEdges() const;
    /**
     * @brief Returns the number of edges in to this node. Takes linear time.
     */
    size_t GetInEdgeCount() const;

    /**
     * @brief An iterator range returning the edges out of this node.
     */
    iterator_pair GetOutEdges();
    /**
     * @brief An iterator range returning the edges out of this node.
     */
    const_iterator_pair GetOutEdges() const;
    /**
     * @brief Returns the number of edges out from this node. Takes linear
     * time.
     */
    size_t GetOutEdgeCount() const;

    bool Serialise(Archive::Item& arc);

    /**
     * @brief Move the node, centering it on the given point.
     */
    virtual void SetPosition(const wxPoint& pt);
    template <class T> void SetPosition(const wxPoint& pt);
    /**
     * @brief Resize the node.
     */
    virtual void SetSize(const wxSize& size);
    template <class T> void SetSize(const wxSize& size);

    /**
     * @brief Set a shape object from the underlying graphics library that
     * will be used to render this edge on the graph control.
     *
     * This makes user code dependent on the particular underlying graphics
     * library. To avoid a dependency, <code>SetStyle()</code> can be used
     * instead to select from a limit range of prefined appearances. Or for
     * more control <code>OnDraw()</code> can be overridden.
     *
     * Invalidates any iterators pointing to this element.
     */
    void SetShape(wxShape *shape);

    /**
     * @brief This can be overridden to give the node a custom shape.
     *
     * It is only called when the style has been set to
     * <code>Style_Custom</code>, and should return a point the perimeter
     * intersects the line between the two points <code>inside</code> and
     * <code>outside</code>.
     *
     * This can be used together with <code>OnDraw()</code> to customise the
     * appearance of a node in a way independent of the underlying graphics
     * library.
     *
     * @param inside A point inside the node's bounds.
     * @param outside A point outside the node's bounds.
     * @returns     A point on the node's perimeter on the line between
     *              inside and outside.
     */
    virtual wxPoint GetPerimeterPoint(const wxPoint& inside,
                                      const wxPoint& outside) const;

protected:
    virtual void DoSelect(bool select);
    virtual void UpdateShape();
    virtual void UpdateShapeTextColour();
    /**
     * @brief Overridable called from Layout().
     */
    virtual void OnLayout(wxDC&) { }
    /**
     * @brief Calculates the positions of any text labels, icons, etc.
     * within the node.
     *
     * This is called by methods that affect the appearance of the node, such
     * as SetSize(), SetText() and SetFont(), to give the node the chance
     * to adjust its layout. It calls OnLayout() to allow derived classes
     * to take care of any custom features they add.
     */
    virtual void Layout();

private:
    wxColour m_textcolour;
    wxString m_text;
    wxFont m_font;

    DECLARE_DYNAMIC_CLASS(GraphNode)
};

// Inline definitions

template <class T> void GraphNode::SetPosition(const wxPoint& pt)
{
    SetPosition(Pixels::From<T>(pt, GetDPI()));
}
template <class T> void GraphNode::SetSize(const wxSize& size)
{
    SetSize(Pixels::From<T>(size, GetDPI()));
}

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
 * The GraphCtrl owns a canvas from the underlying graphics library as a
 * child window. A class derived from GraphCtrl can handle canvas events
 * by getting the canvas window with GetCanvas(), and connecting to the
 * required event. For example:
 *
 * @code
 *  wxWindow *canvas = GetCanvas();
 *
 *  canvas->SetBackgroundStyle(wxBG_STYLE_CUSTOM);
 *  canvas->Connect(wxEVT_ERASE_BACKGROUND,
 *                  wxEraseEventHandler(ProjectDesigner::OnCanvasBackground),
 *                  NULL, this);
 * @endcode
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
    impl::Initialisor m_initalise;
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
    virtual GraphNode *Add(GraphNode *node,
                           wxPoint pt = wxPoint(),
                           wxSize size = wxSize());
    template <class T> GraphNode *Add(GraphNode *node,
                                      wxPoint pt = wxPoint(),
                                      wxSize size = wxSize());
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
     * @brief Finds an empty space for a new node.
     *
     * Searches in a grid like pattern searching across then down line by
     * line. Begins from a default start position below any existing
     * connected nodes.
     *
     * @param spacing The grid spacing for the search pattern.
     * @param columns The columns for the grid pattern or 0 for the default.
     */
    wxPoint FindSpace(const wxSize& spacing, int columns = 0);
    /**
     * @brief Finds an empty space for a new node.
     *
     * Searches in a grid like pattern starting at <code>position</code>,
     * searching across then down line by line.
     *
     * @param position Start searching at this point.
     * @param spacing The grid spacing for the search pattern.
     * @param columns The columns for the grid pattern or 0 for the default.
     */
    wxPoint FindSpace(const wxPoint& position,
                      const wxSize& spacing,
                      int columns = 0);

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
     * @brief Returns the number of nodes in the graph. Takes linear time.
     */
    size_t GetNodeCount() const;
    /**
     * @brief Returns the number of elements in the graph. Takes linear time.
     */
    size_t GetElementCount() const;
    /**
     * @brief Returns the number of elements in the current selection. Takes
     * linear time.
     */
    size_t GetSelectionCount() const;
    /**
     * @brief Returns the number of nodes in the current selection. Takes
     * linear time.
     */
    size_t GetSelectionNodeCount() const;

    /**
     * @brief Write a text representation of the graph and all its elements
     * or a subrange of them.
     */
    virtual bool Serialise(wxOutputStream& out,
                           const iterator_pair& range = iterator_pair());
    virtual bool Serialise(Archive& archive,
                           const iterator_pair& range = iterator_pair());
    /**
     * @brief Load a serialised graph.
     */
    virtual bool Deserialise(wxInputStream& in);
    virtual bool Deserialise(Archive& archive);

    /**
     * @brief Import serialised elements into the current graph.
     */
    virtual bool DeserialiseInto(wxInputStream& in, const wxPoint& pt);
    virtual bool DeserialiseInto(Archive& archive, const wxPoint& pt);

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
    template <class T> void SetGridSpacing(int spacing);
    /**
     * @brief The spacing of the grid used when SetSnapToGrid is switched on.
     */
    virtual int GetGridSpacing() const;
    template <class T> int GetGridSpacing() const;

    /** @brief Undo the last operation. Not yet implemented. */
    virtual void Undo() { wxFAIL; }
    /** @brief Redo the last Undo. Not yet implemented. */
    virtual void Redo() { wxFAIL; }

    /**
     * @brief Indicates the previous operation could be undone with Undo.
     * Not yet implemented.
     */
    virtual bool CanUndo() const { wxFAIL; return false; }
    /**
     * @brief Indicates the previous Undo could be redone with Redo.
     * Not yet implemented.
     */
    virtual bool CanRedo() const { wxFAIL; return false; }

    /**
     * @brief Cut the current selection to the clipboard.
     * Not yet implemented.
     */
    virtual bool Cut() { wxFAIL; return false; }
    /**
     * @brief Copy the current selection to the clipboard.
     * Not yet implemented.
     */
    virtual bool Copy() { wxFAIL; return false; }
    /**
     * @brief Paste from the clipboard, replacing the current selection.
     * Not yet implemented.
     */
    virtual bool Paste() { wxFAIL; return false; }
    /** @brief Delete the nodes and edges in the current selection. */
    void Clear() { Delete(GetSelection()); }

    virtual bool CanCut() const { wxFAIL; return false; }
    /**
     * @brief Indicates that the current selection is non-empty.
     * Not yet implemented.
     */
    virtual bool CanCopy() const { wxFAIL; return false; }
    /**
     * @brief Indicates that there is graph data in the clipboard.
     * Not yet implemented.
     */
    virtual bool CanPaste() const { wxFAIL; return false; }
    virtual bool CanClear() const;

    /**
     * @brief Returns a bounding rectange for all the elements currently
     * in the graph.
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

    /* helper to send an event to the graph event handler. */
    void SendEvent(wxEvent& event);

    wxSize GetDPI() const { return m_dpi; }

    void SetFont(const wxFont& font);
    wxFont GetFont() const;

protected:
    virtual GraphNode *DoAdd(GraphNode *node,
                             wxPoint pt,
                             wxSize size);
    virtual GraphEdge *DoAdd(GraphNode& from,
                             GraphNode& to,
                             GraphEdge *edge = NULL);

private:
    friend void GraphCtrl::SetGraph(Graph *graph);

    void SetCanvas(impl::GraphCanvas *canvas);
    impl::GraphCanvas *GetCanvas() const;
    void DoDelete(GraphElement *element);

    impl::Initialisor m_initalise;
    impl::GraphDiagram *m_diagram;
    mutable wxRect m_rcBounds;
    wxEvtHandler *m_handler;
    wxSize m_dpi;
    double m_gridSpacing;

    DECLARE_DYNAMIC_CLASS(Graph)
    DECLARE_NO_COPY_CLASS(Graph)
};

// Inline definitions

template <class T>
GraphNode *Graph::Add(GraphNode *node, wxPoint pt, wxSize size)
{
    return Add(node, Pixels::From<T>(pt, m_dpi),
                     Pixels::From<T>(size, m_dpi));
}

template <class T> void Graph::SetGridSpacing(int spacing)
{
    if (m_dpi == wxSize() && T::Inch)
        m_gridSpacing = double(spacing) / T::Inch;
    else
        SetGridSpacing(Pixels::From<T>(spacing, m_dpi.y));
}

template <class T> int Graph::GetGridSpacing() const
{
    if (m_dpi == wxSize() && T::Inch)
        return int(m_gridSpacing * T::Inch + 0.5);
    else
        return Pixels::To<T>(GetGridSpacing(), m_dpi.y);
}

/**
 * @brief Graph event
 */
class GraphEvent : public wxNotifyEvent
{
public:
    /**
     * @brief A list type used by EVT_GRAPH_CONNECT and
     * EVT_GRAPH_CONNECT_FEEDBACK to provide a list of all the source nodes.
     */
    typedef std::list<GraphNode*> NodeList;

    GraphEvent(wxEventType commandType = wxEVT_NULL, int winid = 0);
    GraphEvent(const GraphEvent& event);

    virtual wxEvent *Clone() const      { return new GraphEvent(*this); }

    /**
     * @brief The node being added, deleted, clicked, etc..
     */
    void SetNode(GraphNode *node)       { m_node = node; }
    /**
     * @brief Set by EVT_GRAPH_CONNECT and EVT_GRAPH_CONNECT_FEEDBACK to
     * indicate the target node.
     */
    void SetTarget(GraphNode *node)     { m_target = node; }
    /**
     * @brief The edge being added, deleted, clicked, etc..
     */
    void SetEdge(GraphEdge *edge)       { m_edge = edge; }
    /**
     * @brief The cursor position for mouse related events.
     */
    void SetPosition(const wxPoint& pt) { m_pos = pt; }
    /**
     * @brief A list provided by EVT_GRAPH_CONNECT and
     * EVT_GRAPH_CONNECT_FEEDBACK of all the source nodes.
     */
    void SetSources(NodeList& sources)  { m_sources = &sources; }

    /**
     * @brief The node being added, deleted, clicked, etc..
     */
    GraphNode *GetNode() const          { return m_node; }
    /**
     * @brief Set by EVT_GRAPH_CONNECT and EVT_GRAPH_CONNECT_FEEDBACK to
     * indicate the target node.
     */
    GraphNode *GetTarget() const        { return m_target; }
    /**
     * @brief The edge being added, deleted, clicked, etc..
     */
    GraphEdge *GetEdge() const          { return m_edge; }
    /**
     * @brief The cursor position for mouse related events.
     */
    wxPoint GetPosition() const         { return m_pos; }
    /**
     * @brief A list provided by EVT_GRAPH_CONNECT and
     * EVT_GRAPH_CONNECT_FEEDBACK of all the source nodes.
     */
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

/**
 * @brief Fired when a node is about to be added to the graph.
 *
 * <code>GraphEvent::GetNode()</code> returns the node that will be added.
 * Vetoing the event cancels the addition of the node and deletes it.
 */
#define EVT_GRAPH_NODE_ADD(fn) DECLARE_GRAPH_EVT0(Node_Add, fn)
/**
 * @brief Fired when a node is about to be deleted from the graph.
 *
 * <code>GraphEvent::GetNode()</code> returns the node that will be
 * deleted. Vetoing the event cancels the deletion of the node.
 */
#define EVT_GRAPH_NODE_DELETE(fn) DECLARE_GRAPH_EVT0(Node_Delete, fn)

/**
 * @brief Fired when a edge is about to be added to the graph.
 *
 * The parameters passed to <code>Graph::Add(GraphNode& from, GraphNode& to,
 * GraphEdge *edge)</code> are available from the <code>GraphEvent</code>'s
 * <code>GetNode()</code>, <code>GetTarget()</code> and
 * <code>GetEdge()</code> methods.
 *
 * <code>GetEdge()</code> will return <code>NULL</code> if the
 * <code>edge</code> parameter of <code>Add()</code> was <code>NULL</code>.
 * In this case, after the event handler returns a <code>GraphEdge</code>
 * object is created to be added to the graph. Alternatively, To override the
 * default attributes of the edge, or to use a derived type instead a
 * <code>GraphEdge</code>, the handler can create an edge object and set it
 * with <code>GraphEvent::SetEdge()</code>.
 *
 * Vetoing the event cancels the addition of the edge and deletes it.
 */
#define EVT_GRAPH_EDGE_ADD(fn) DECLARE_GRAPH_EVT0(Edge_Add, fn)
/**
 * @brief Fired when an edge is about to be deleted from the graph.
 *
 * <code>GraphEvent::GetNode()</code> returns the edge that will be
 * deleted. Vetoing the event cancels the deletion of the edge.
 */
#define EVT_GRAPH_EDGE_DELETE(fn) DECLARE_GRAPH_EVT0(Edge_Delete, fn)

/**
 * @brief Handles both EVT_GRAPH_NODE_ADD and EVT_GRAPH_EDGE_ADD with
 * a single event handler.
 */
#define EVT_GRAPH_ELEMENT_ADD(fn) EVT_GRAPH_NODE_ADD(fn) EVT_GRAPH_EDGE_ADD(fn)
/**
 * @brief Handles both EVT_GRAPH_NODE_DELETE and EVT_GRAPH_EDGE_DELETE with
 * a single event handler.
 */
#define EVT_GRAPH_ELEMENT_DELETE(fn) EVT_GRAPH_NODE_DELETE(fn) EVT_GRAPH_EDGE_DELETE(fn)

/**
 * @brief Fires during node dragging each time the cursor hovers over
 * a potential target node, and allows the application to decide whether
 * dropping here would create a link.
 *
 * GetSources() returns a list of source nodes, and GetTarget() returns the
 * target node.  Removing nodes from the sources list disallows just that
 * connection while permitting other sources to connect. Vetoing the event
 * disallows all connections (it's equivalent to clearing the list).
 */
#define EVT_GRAPH_CONNECT_FEEDBACK(fn) DECLARE_GRAPH_EVT0(Connect_Feedback, fn)
/**
 * @brief This event fires when nodes have been dropped on a target node.
 *
 * GetSources() returns a list of source nodes, and GetTarget() returns the
 * target node.  Removing a node from the sources list disallows just that
 * connection while permitting other sources to connect. Vetoing the event
 * disallows all connections (it's equivalent to clearing the list).
 */
#define EVT_GRAPH_CONNECT(fn) DECLARE_GRAPH_EVT0(Connect, fn)

// GraphCtrl Events

/**
 * @brief Fires when a node is clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_NODE_CLICK(id, fn) DECLARE_GRAPH_EVT1(Node_Click, id, fn)
/**
 * @brief Fires when a node is double clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_NODE_ACTIVATE(id, fn) DECLARE_GRAPH_EVT1(Node_Activate, id, fn)
/**
 * @brief Fires when a node is right clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_NODE_MENU(id, fn) DECLARE_GRAPH_EVT1(Node_Menu, id, fn)

/**
 * @brief Fires when an edge is clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_EDGE_CLICK(id, fn) DECLARE_GRAPH_EVT1(Edge_Click, id, fn)
/**
 * @brief Fires when an edge is double clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_EDGE_ACTIVATE(id, fn) DECLARE_GRAPH_EVT1(Edge_Activate, id, fn)
/**
 * @brief Fires when an edge is right clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_EDGE_MENU(id, fn) DECLARE_GRAPH_EVT1(Edge_Menu, id, fn)

/**
 * @brief Handles both EVT_GRAPH_NODE_CLICK and EVT_GRAPH_EDGE_CLICK with
 * a single event handler.
 */
#define EVT_GRAPH_ELEMENT_CLICK(id, fn) EVT_GRAPH_NODE_CLICK(id, fn) EVT_GRAPH_EDGE_CLICK(id, fn)
/**
 * @brief Handles both EVT_GRAPH_NODE_ACTIVATE and EVT_GRAPH_EDGE_ACTIVATE
 * with a single event handler.
 */
#define EVT_GRAPH_ELEMENT_ACTIVATE(id, fn) EVT_GRAPH_NODE_ACTIVATE(id, fn) EVT_GRAPH_EDGE_ACTIVATE(id, fn)
/**
 * @brief Handles both EVT_GRAPH_NODE_MENU and EVT_GRAPH_EDGE_MENU with
 * a single event handler.
 */
#define EVT_GRAPH_ELEMENT_MENU(id, fn) EVT_GRAPH_NODE_MENU(id, fn) EVT_GRAPH_EDGE_MENU(id, fn)

#endif // GRAPHCTRL_H
