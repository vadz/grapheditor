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
class wxTipWindow;

/**
 * @brief TT-Solutions
 */
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
    /** @cond */
    class GraphIteratorImpl;
    class GraphDiagram;
    class GraphCanvas;

    enum IteratorFilter { All, Selected, InEdges, OutEdges };

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

    /** @endcond */

} // namespace impl

/**
 * @brief Iterator class template for graph elements.
 *
 * Graph elements are enumerated using iterator types such as
 * @c GraphIterator<GraphNode> and @c GraphIterator<GraphEdge>, etc..
 *
 * In general, an iterator type is assignable to another if a pointer to the
 * types would be assignable. For example, a
 * <code>GraphIterator<GraphNode></code> is assignable to a
 * <code>GraphIterator<GraphElement></code>, but not vice versa.
 *
 * The graph methods that return iterators take a template parameter allow the
 * type of the iterator to be specified. For example @c GetNodes<ProjectNode>
 * would return @c GraphIterator<ProjectNode> iterators.
 *
 * Methods that return iterators return a begin/end pair in a
 * <code>std::pair</code>.  These can be assigned to a pair of variables
 * using the '<code>tie</code>' function, so the usual idiom for using them
 * is:
 *
 * @code
 *  GraphIterator<ProjectNode> it, end;
 *
 *  for (tie(it, end) = m_graph->GetSelection<ProjectNode>(); it != end; ++it)
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
 * tie() \n
 * IterPair \n
 * Graph::GetNodes() \n
 * Graph::GetSelection() \n
 * GraphNode::GetEdges() \n
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
    typedef typename std::pair<GraphIterator, GraphIterator> pair;

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
 * @brief A shorter synonym for <code>std::pair< GraphIterator<T>,
 * GraphIterator<T> ></code>.
 */
template <class T>
struct IterPair : std::pair< GraphIterator<T>, GraphIterator<T> >
{
    /** @cond */
    typedef std::pair< GraphIterator<T>, GraphIterator<T> > Base;

    IterPair()
    { }

    template <class U>
    IterPair(const std::pair< GraphIterator<U>, GraphIterator<U> >& other)
      : Base(other)
    { }
    /** @endcond */
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
     * @brief Deselects this element.
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
     * @brief Save or load this node's attributes.
     *
     * Can be overridden in a derived class to handle any additional
     * attributes.
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
    /**
     * @brief Get the associated graphic object from the underlying graphics
     * library, creating it if necessary.
     *
     * Invalidates any iterators pointing to this element.
     */
    GraphShape *EnsureShape() { return DoEnsureShape(); }

    /**
     * @brief Returns the graph that this element has been added to, or
     * NULL if it has not be added.
     */
    virtual Graph *GetGraph() const;

    /**
     * @brief Returns the size of the graph element in graph coordinates.
     *
     * @tparam T @c Points or @c Twips specifying the units of the returned
     * size.  If omitted defaults to pixels.
     */
    template <class T> wxSize GetSize() const;
    /** @cond */
    virtual wxSize GetSize() const;
    /** @endcond */

    /**
     * @brief Returns the position of the graph element in graph coordinates.
     *
     * @tparam T @c Points or @c Twips specifying the units of the returned
     * position.  If omitted defaults to pixels.
     */
    template <class T> wxPoint GetPosition() const;
    /** @cond */
    virtual wxPoint GetPosition() const;
    /** @endcond */

    /**
     * @brief Returns the bounding rectangle of the graph element in graph
     * coordinates.
     *
     * @tparam T @c Points or @c Twips specifying the units of the returned
     * rectangle.  If omitted defaults to pixels.
     */
    template <class T> wxRect GetBounds() const;
    /** @cond */
    virtual wxRect GetBounds() const;
    /** @endcond */

    /**
     * @brief Invalidates the bounds of the element so that it redraws the
     * next time its graph control handles a wxEVT_PAINT event.
     */
    virtual void Refresh();

    /**
     * @brief This is called by methods that affect the appearance of the
     * element.
     *
     * Such as SetSize(), SetText() and SetFont(), to give the element the
     * chance to adjust its layout. The default implementation does nothing.
     */
    virtual void Layout() = 0;

    /** @brief Overridable returning the pen that will be used. */
    virtual wxPen GetPen() const        { return wxPen(m_colour); }
    /** @brief Overridable returning the brush that will be used. */
    virtual wxBrush GetBrush() const    { return wxBrush(m_bgcolour); }

protected:
    /** @cond */
    virtual void DoSelect(bool select);
    virtual void UpdateShape() = 0;
    virtual void SetShape(GraphShape *shape);
    virtual GraphShape *DoGetShape() const { return m_shape; }
    virtual GraphShape *DoEnsureShape();
    /** @endcond */

    /** @brief The DPI of the graph's nominal pixels. */
    virtual wxSize GetDPI() const;

private:
    impl::Initialisor m_initalise;

    wxColour m_colour;
    wxColour m_bgcolour;

    int m_style;
    GraphShape *m_shape;

    DECLARE_ABSTRACT_CLASS(GraphElement)
};

// Inline definitions

template <class T> wxSize GraphElement::GetSize() const
{
    return Pixels::To<T>(GetSize(), GetDPI());
}

template <class T> wxPoint GraphElement::GetPosition() const
{
    return Pixels::To<T>(GetPosition(), GetDPI());
}

template <class T> wxRect GraphElement::GetBounds() const
{
    return Pixels::To<T>(GetBounds(), GetDPI());
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
    typedef iterator::pair iterator_pair;
    /** @brief A begin/end pair of iterators. */
    typedef const_iterator::pair const_iterator_pair;

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

    //@{
    /** @brief Size of the arrow head, if present. */
    virtual void SetArrowSize(int size);
    virtual int GetArrowSize() const { return m_arrowsize; }
    //@}

    //@{
    /** @brief Width of the line. */
    virtual void SetLineWidth(int width) { m_linewidth = width; Refresh(); }
    virtual int GetLineWidth() const { return m_linewidth; }
    //@}

    //@{
    /**
     * @brief An iterator range returning the two nodes this edge connects.
     *
     * @tparam T A node type. If given returns only nodes of that type. If
     * omitted defaults to @c GraphNode.
     */
    template <class T> IterPair<T> GetNodes() { return Iter<T>(); }
    template <class T> IterPair<const T> GetNodes() const;
    //@}
    /** @cond */
    iterator_pair GetNodes() { return Iter<GraphNode>(); }
    const_iterator_pair GetNodes() const { return Iter<const GraphNode>(); }
    /** @endcond */

    /**
     * @brief Returns the number of nodes this edge connects, i.e. two if the
     * edge has been added to a graph.
     */
    size_t GetNodeCount() const;

    /**
     * @brief Returns the first of the two nodes this edge connects.
     *
     * @tparam T A node type. If given returns only nodes of that type. If
     * omitted defaults to @c GraphNode.
     */
    template <class T> T *GetFrom() const;
    /** @cond */
    virtual GraphNode *GetFrom() const;
    /** @endcond */

    /**
     * @brief Returns the second of the two nodes this edge connects.
     *
     * @tparam T A node type. If given returns only nodes of that type. If
     * omitted defaults to @c GraphNode.
     */
    template <class T> T *GetTo() const;
    /** @cond */
    virtual GraphNode *GetTo() const;
    /** @endcond */

    /**
     * @brief Serialise or deserialise this edge's attributes.
     *
     * Can be overridden in a derived class to handle any additional
     * attributes.
     */
    bool Serialise(Archive::Item& arc);

    /**
     * @brief Get the associated graphic object from the underlying graphics
     * library.
     */
    GraphLineShape *GetShape() const;
    /**
     * @brief Get the associated graphic object from the underlying graphics
     * library, creating it if necessary.
     *
     * Invalidates any iterators pointing to this element.
     */
    GraphLineShape *EnsureShape();

    /**
     * @brief Set a shape object from the underlying graphics library that
     * will be used to render this edge on the graph control.
     *
     * This makes user code dependent on the particular underlying graphics
     * library. To avoid a dependency, <code>SetStyle()</code> can be used
     * instead to select from a limit range of predefined appearances. Or for
     * more control <code>OnDraw()</code> can be overridden.
     *
     * Invalidates any iterators pointing to this element.
     */
    virtual void SetShape(GraphLineShape *shape);

    /**
     * @brief This is called by methods that affect the appearance of the
     * element.
     *
     * Such as SetSize(), SetText() and SetFont(), to give the element the
     * chance to adjust its layout. The default implementation does nothing.
     */
    virtual void Layout() { }

    /** @brief Overridable returning the pen that will be used. */
    wxPen GetPen() const { return wxPen(GetColour(), m_linewidth); }

protected:
    /** @cond */
    void UpdateShape() { }
    bool MoveFront();
    /** @endcond */

private:
    impl::GraphIteratorImpl *IterImpl(
        GraphLineShape *line, wxClassInfo *classinfo, bool end) const;

    template <class T> IterPair<T> Iter() const;

    int m_arrowsize;
    int m_linewidth;

    DECLARE_DYNAMIC_CLASS(GraphEdge)
};

template <class T> IterPair<T> GraphEdge::Iter() const
{
    GraphLineShape *line = GetShape();

    return std::make_pair(
        GraphIterator<T>(IterImpl(line, CLASSINFO(T), false)),
        GraphIterator<T>(IterImpl(line, CLASSINFO(T), true)));
}

template <class T> IterPair<const T> GraphEdge::GetNodes() const
{
    return Iter<const T>();
}

template <class T> T *GraphEdge::GetFrom() const
{
    return dynamic_cast<T*>(GetFrom());
}

template <class T> T *GraphEdge::GetTo() const
{
    return dynamic_cast<T*>(GetTo());
}

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
    typedef iterator::pair iterator_pair;
    /** @brief A begin/end pair of iterators. */
    typedef const_iterator::pair const_iterator_pair;

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

    //@{
    /** @brief The node's main text label. */
    virtual void SetText(const wxString& text);
    virtual wxString GetText() const { return m_text; }
    //@}

    //@{
    /** @brief Text for the node's tooltip. */
    virtual void SetToolTip(const wxString& text) { m_tooltip = text; }
    virtual wxString GetToolTip(const wxPoint& pt = wxPoint()) const;
    //@}

    //@{
    /**
     * @brief A text value indicating the node's rank (row in layout).
     *
     * Nodes given the same rank text will be placed at the same height when
     * automatically laid out.
     */
    virtual void SetRank(const wxString& name) { m_rank = name; }
    virtual wxString GetRank() const { return m_rank; }
    //@}

    //@{
    /** @brief The colour of the node's text. */
    virtual void SetTextColour(const wxColour& colour);
    virtual wxColour GetTextColour() const { return m_textcolour; }
    //@}

    //@{
    /**
     * @brief The node's font.
     *
     * If no font is set for the node it inherits the font of the graph.
     */
    virtual void SetFont(const wxFont& font);
    virtual wxFont GetFont() const;
    //@}

    /**
     * @brief A number from the Style enumeration indicating the node's
     * appearance.
     *
     * Invalidates any iterators pointing to this element.
     */
    virtual void SetStyle(int style);

    //@{
    /**
     * @brief An iterator range returning the edges connecting to this node.
     *
     * @tparam T An edge type. If given returns all the edges of that type
     * currently connected to this node. If omitted defaults to @c GraphEdge.
     */
    template <class T> IterPair<T> GetEdges() { return Iter<T>(); }
    template <class T> IterPair<const T> GetEdges() const;
    //@}
    /** @cond */
    iterator_pair GetEdges() { return Iter<GraphEdge>(); }
    const_iterator_pair GetEdges() const { return Iter<const GraphEdge>(); }
    /** @endcond */

    /**
     * @brief Returns the number of edges connecting to this node.
     */
    size_t GetEdgeCount() const;

    //@{
    /**
     * @brief An iterator range returning the edges into this node.
     *
     * @tparam T An edge type. If given returns all the edges of that type
     * currently incoming to this node. If omitted defaults to @c GraphEdge.
     */
    template <class T> IterPair<T> GetInEdges();
    template <class T> IterPair<const T> GetInEdges() const;
    //@}
    /** @cond */
    iterator_pair GetInEdges() { return Iter<GraphEdge>(impl::InEdges); }
    inline const_iterator_pair GetInEdges() const;
    /** @endcond */

    /**
     * @brief Returns the number of edges in to this node. Takes linear time.
     */
    size_t GetInEdgeCount() const;

    //@{
    /**
     * @brief An iterator range returning the edges out of this node.
     *
     * @tparam T An edge type. If given returns all the edges of that type
     * currently outgoing from this node. If omitted defaults to @c GraphEdge.
     */
    template <class T> IterPair<T> GetOutEdges();
    template <class T> IterPair<const T> GetOutEdges() const;
    //@}
    /** @cond */
    iterator_pair GetOutEdges() { return Iter<GraphEdge>(impl::OutEdges); }
    inline const_iterator_pair GetOutEdges() const;
    /** @endcond */

    /**
     * @brief Returns the number of edges out from this node. Takes linear
     * time.
     */
    size_t GetOutEdgeCount() const;

    /**
     * @brief Save or load this node's attributes.
     *
     * Can be overridden in a derived class to handle any additional
     * attributes.
     */
    bool Serialise(Archive::Item& arc);

    /**
     * @brief Move the node, centering it on the given point.
     *
     * @tparam T @c Points or @c Twips specifying the units of @c pt. If
     * omitted defaults to pixels.
     */
    template <class T> void SetPosition(const wxPoint& pt);
    /** @cond */
    virtual void SetPosition(const wxPoint& pt);
    /** @endcond */

    /**
     * @brief Resize the node.
     *
     * @tparam T @c Points or @c Twips specifying the units of @c size. If
     * omitted defaults to pixels.
     */
    template <class T> void SetSize(const wxSize& size);
    /** @cond */
    virtual void SetSize(const wxSize& size);
    /** @endcond */

    /**
     * @brief Set a shape object from the underlying graphics library that
     * will be used to render this edge on the graph control.
     *
     * This makes user code dependent on the particular underlying graphics
     * library. To avoid a dependency, <code>SetStyle()</code> can be used
     * instead to select from a limit range of predefined appearances. Or for
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
    /** @cond */
    virtual void DoSelect(bool select);
    virtual void UpdateShape();
    virtual void UpdateShapeTextColour();
    virtual void DoSetSize(wxDC& dc, const wxSize& size);
    /** @endcond */

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
    wxList *GetLines() const;

    impl::GraphIteratorImpl *IterImpl(
        const wxList::iterator& begin,
        const wxList::iterator& end,
        wxClassInfo *classinfo,
        int which) const;

    template <class T> IterPair<T> Iter(int which = impl::All) const;

    wxColour m_textcolour;
    wxString m_text;
    wxString m_tooltip;
    wxString m_rank;
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

template <class T> IterPair<const T> GraphNode::GetEdges() const
{
    return Iter<const T>();
}

template <class T> IterPair<T> GraphNode::GetInEdges()
{
    return Iter<T>(impl::InEdges);
}

GraphNode::const_iterator_pair GraphNode::GetInEdges() const
{
    return Iter<const GraphEdge>(impl::InEdges);
}

template <class T> IterPair<const T> GraphNode::GetInEdges() const
{
    return Iter<const T>(impl::InEdges);
}

template <class T> IterPair<T> GraphNode::GetOutEdges()
{
    return Iter<T>(impl::OutEdges);
}

GraphNode::const_iterator_pair GraphNode::GetOutEdges() const
{
    return Iter<const GraphEdge>(impl::OutEdges);
}

template <class T> IterPair<const T> GraphNode::GetOutEdges() const
{
    return Iter<const T>(impl::OutEdges);
}

template <class T> IterPair<T> GraphNode::Iter(int which) const
{
    wxList *list = GetLines();
    wxList::iterator begin, end;

    if (list) {
        begin = list->begin();
        end = list->end();
    }

    return std::make_pair(
        GraphIterator<T>(IterImpl(begin, end, CLASSINFO(T), which)),
        GraphIterator<T>(IterImpl(end, end, CLASSINFO(T), which)));
}

/**
 * @brief A control for interactive editing of a Graph object.
 *
 * The GraphCtrl is associated with a Graph object by calling SetGraph.
 * For example, you frame's OnInit() method might contain:
 *
 * @code
 *  m_graph = new Graph(this);
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
    virtual void SetZoom(double percent);
    /**
     * @brief Scales the image by the given percentage, fixing the given
     * point in the viewport.
     */
    virtual void SetZoom(double percent, const wxPoint& pt);
    /**
     * @brief Returns the current scaling as a percentage.
     */
    virtual double GetZoom() const;

    /**
     * @brief Sets the Graph object that this GraphCtrl will operate on.
     * The GraphCtrl does not take ownership.
     */
    virtual void SetGraph(Graph *graph);
    /**
     * @brief Returns the Graph object associated with this GraphCtrl.
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
     * @brief Scroll to the top/bottom/left/right side of the graph.
     *
     * Affected by @c SetMargin().
     */
    virtual void ScrollTo(int side);
    /** @brief Centre the given graph coordinate in the view. */
    virtual void ScrollTo(const wxPoint& ptGraph);
    /** @brief Returns the centre of the view in graph coordinates. */
    virtual wxPoint GetScrollPosition() const;
    /**
     * @brief Home the graph in the view, make the topmost node visible.
     *
     * Affected by @c SetMargin().
     */
    virtual void Home();
    /**
     * @brief Fit the Graph to the view.
     *
     * Affected by @c SetMargin().
     */
    virtual void Fit();

    /**
     * @brief The kind of border the scrollbars leave around the graph.
     *
     * @see SetBorderType() \n SetBorder() \n SetMargin()
     */
    enum BorderType {
        Percentage_Border, /**< Percentage of the control's client area. */
        Graph_Border,      /**< Graph pixels, scales with zooming. */
        Ctrl_Border        /**< Control pixels, does not scale with zooming. */
    };

    //@{
    /**
     * @brief The kind of border the scrollbars leave around the graph.
     *
     * @see BorderType \n SetBorder() \n SetMargin()
     */
    void SetBorderType(BorderType type);
    BorderType GetBorderType() const;
    //@}

    //@{
    /**
     * @brief Extra border left around the graph by the scrollbars.
     *
     * @c SetMargin() can be used to set a margin around the graph. It
     * affects the range of the scrollbars and also the graph's home position
     * and the the scaling of @c Fit().
     *
     * @c %SetBorder() on the other hand, applies to the scroll range without
     * affecting anything else. The greater of @c SetMargin() and @c
     * %SetBorder() applies as far as the scroll range is concerned.
     *
     * @tparam T Must be omitted when @c BorderType is @c Percentage_Border.
     * Otherwise it can be @c Points or @c Twips specifying the units of @c
     * size, or omitted for pixels.
     *
     * @see BorderType \n SetBorderType() \n SetMargin()
     */
    template <class T> void SetBorder(const wxSize& size);
    template <class T> wxSize GetBorder() const;
    //@}
    /** @cond */
    void SetBorder(const wxSize& size);
    wxSize GetBorder() const;
    /** @endcond */

    //@{
    /**
     * @brief Margin left around the graph by Home/End keys.
     *
     * @c %SetMargin() can be used to set a margin around the graph. It
     * affects the range of the scrollbars and also the graph's home position
     * and the scaling of @c Fit().
     *
     * @c SetBorder() on the other hand, applies to the scroll range without
     * affecting anything else. The greater of @c %SetMargin() and @c
     * SetBorder() applies as far as the scroll range is concerned.
     *
     * @tparam T @c Points or @c Twips specifying the units of @c size.  If
     * omitted defaults to pixels.
     *
     * @see SetBorder()
     */
    template <class T> void SetMargin(const wxSize& size);
    template <class T> wxSize GetMargin() const;
    //@}
    /** @cond */
    void SetMargin(const wxSize& size);
    wxSize GetMargin() const;
    /** @endcond */

    //@{
    /** @brief The delay in milliseconds before nodes' tooltips are shown. */
    void SetToolTipDelay(int millisecs) { m_tipdelay = millisecs; }
    int GetToolTipDelay() const { return m_tipdelay; }
    //@}

    /**
     * @brief What happens when nodes are dragged (can be ored together).
     *
     * @see SetLeftDragMode() \n SetRightDragMode()
     */
    enum DragMode {
        Drag_Disable = 0,       /**< Dragging does nothing. */
        Drag_Move    = 1 << 0,  /**< Dragging moves nodes. */
        Drag_Connect = 1 << 1   /**< Dragging connects nodes. */
    };

    //@{
    /**
     * @brief Determines what happens when nodes are dragged.
     *
     * Values from the @c #DragMode enum, bitwise ored.
     */
    static void SetLeftDragMode(int mode)   { sm_leftDrag = mode; }
    static int GetLeftDragMode()            { return sm_leftDrag; }
    static void SetRightDragMode(int mode)  { sm_rightDrag = mode; }
    static int GetRightDragMode()           { return sm_rightDrag; }
    //@}

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

    /** @cond */
    void OnSize(wxSizeEvent& event);
    void OnChar(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnTipTimer(wxTimerEvent& event);
    /** @endcond */

    /** @cond */
    /* default value for the constructor's name parameter. */
    static const wxChar DefaultName[];
    /** @endcond */

protected:
    /** @cond */
    virtual wxSize GetDPI() const;
    /** @endcond */

private:
    void ScrollHomeEnd(bool home);

    impl::Initialisor m_initalise;
    impl::GraphCanvas *m_canvas;
    Graph *m_graph;
    wxTimer m_tiptimer;
    int m_tipdelay;
    wxTipWindow *m_tipwin;
    static int sm_leftDrag;
    static int sm_rightDrag;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(GraphCtrl)
    DECLARE_NO_COPY_CLASS(GraphCtrl)
};

// Inline definitions

template <class T> wxSize GraphCtrl::GetBorder() const
{
    return Pixels::To<T>(GetBorder(), GetDPI());
}

template <class T> void GraphCtrl::SetBorder(const wxSize& size)
{
    SetBorder(Pixels::From<T>(size, GetDPI()));
}

template <class T> wxSize GraphCtrl::GetMargin() const
{
    return Pixels::To<T>(GetMargin(), GetDPI());
}

template <class T> void GraphCtrl::SetMargin(const wxSize& size)
{
    SetMargin(Pixels::From<T>(size, GetDPI()));
}

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
    typedef iterator::pair iterator_pair;
    /** @brief A begin/end pair of iterators returning nodes and edges. */
    typedef const_iterator::pair const_iterator_pair;
    /** @brief A begin/end pair of iterators returning nodes. */
    typedef node_iterator::pair node_iterator_pair;
    /** @brief A begin/end pair of iterators returning nodes. */
    typedef const_node_iterator::pair const_node_iterator_pair;

    /**
     * @brief Constructor.
     *
     * @param handler The graph's parent, the handler of its events.
     */
    Graph(wxEvtHandler *handler = NULL);
    /** @brief Destructor. */
    ~Graph();

    /** @brief Clear all the graph's data. */
    virtual void New();

    /**
     * @brief Adds a node to the graph. The Graph object takes ownership.
     *
     * @param node The node.
     * @param pt Centre position for the node.
     * @param size Size of the node.
     *
     * @tparam T @c Points or @c Twips specifying the units of @c pt and @c
     * size. If omitted defaults to pixels.
     *
     * @returns A pointer to the node if successful, otherwise the node is
     * deleted and NULL returned.
     */
    template <class T> GraphNode *Add(GraphNode *node,
                                      wxPoint pt = wxPoint(),
                                      wxSize size = wxSize());
    /** @cond */
    virtual GraphNode *Add(GraphNode *node,
                           wxPoint pt = wxPoint(),
                           wxSize size = wxSize());
    /** @endcond */

    /**
     * @brief Adds an edge to the Graph, between the two nodes.
     *
     * If a GraphEdge is supplied via the edge parameter the Graph takes
     * ownership of it; if the edge parameter is omitted an edge object is
     * created implicitly. The nodes must have already been added.
     *
     * @returns A pointer to the edge if successful, otherwise the edge is
     * deleted and NULL returned.
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

    /**
     * @brief Invokes a layout engine to lay out the graph.
     *
     * @param fixed A node in the graph that will not move, defaults to the
     * top leftmost node.
     * @param ranksep The vertical separation in inches.
     * @param nodesep The horizontal separation in inches.
     */
    virtual bool LayoutAll(const GraphNode *fixed = NULL,
                           double ranksep = 0.5,
                           double nodesep = 0.3);
    /**
     * @brief Invokes a layout engine to lay out the subset of the graph
     * specified by the given iterator range.
     *
     * @param range An iterator range of nodes to lay out.
     * @param fixed A node in the graph that will not move defaults to the
     * top leftmost node with an external edge connection.
     * @param ranksep The vertical separation in inches.
     * @param nodesep The horizontal separation in inches.
     */
    virtual bool Layout(const node_iterator_pair& range,
                        const GraphNode *fixed = NULL,
                        double ranksep = 0.5,
                        double nodesep = 0.3);

    /**
     * @brief Finds an empty space for a new node.
     *
     * Searches in a grid like pattern searching across then down line by
     * line. Begins from a default start position below any existing
     * connected nodes.
     *
     * @param spacing The grid spacing for the search pattern.
     * @param columns The columns for the grid pattern or 0 for the default.
     *
     * @tparam T @c Points or @c Twips specifying the units of @c spacing.
     * If omitted defaults to pixels. The return value is always in pixels.
     */
    template <class T>
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
     *
     * @tparam T @c Points or @c Twips specifying the units of @c position and
     * @c spacing.  If omitted defaults to pixels. The return value is always
     * in pixels.
     */
    template <class T>
    wxPoint FindSpace(const wxPoint& position,
                      const wxSize& spacing,
                      int columns = 0);
    /** @cond */
    wxPoint FindSpace(const wxSize& spacing, int columns = 0);
    wxPoint FindSpace(const wxPoint& position,
                      const wxSize& spacing,
                      int columns = 0);
    /** @endcond */

    /**
     * @brief Adds the nodes and edges specified by the given iterator range
     * to the current selection.
     */
    virtual void Select(const iterator_pair& range);
    /** @brief Adds all elements in the graph to the current selection. */
    virtual void SelectAll() { Select(GetElements()); }

    /**
     * @brief Removes the nodes and edges specified by the given iterator
     * range from the current selection.
     */
    virtual void Unselect(const iterator_pair& range);
    /** @brief Removes all elements in the graph from the current selection. */
    virtual void UnselectAll() { Unselect(GetSelection()); }

    //@{
    /**
     * @brief An iterator range returning all the nodes in the graph.
     *
     * @tparam T A node type. If given returns all the nodes of that type
     * in the graph. If omitted defaults to @c GraphNode.
     */
    template <class T> IterPair<T> GetNodes() { return Iter<T, GraphNode>(); }
    template <class T> IterPair<const T> GetNodes() const;
    //@}
    /** @cond */
    node_iterator_pair GetNodes() { return Iter<GraphNode>(); }
    inline const_node_iterator_pair GetNodes() const;
    /** @endcond */

    //@{
    /**
     * @brief An iterator range returning all the nodes and edges in the
     * graph.
     *
     * @tparam T An element type. If given returns all the elements of that
     * type in the graph. If omitted defaults to @c GraphElement.
     */
    template <class T> IterPair<T> GetElements() { return Iter<T>(); }
    template <class T> IterPair<const T> GetElements() const;
    //@}
    /** @cond */
    iterator_pair GetElements() { return Iter<GraphElement>(); }
    inline const_iterator_pair GetElements() const;
    /** @endcond */

    //@{
    /**
     * @brief An iterator range returning all the nodes and edges currently
     * selected.
     *
     * @tparam T An element type. If given returns all the elements of that
     * type in the selection. If omitted defaults to @c GraphElement.
     */
    template <class T> IterPair<T> GetSelection();
    template <class T> IterPair<const T> GetSelection() const;
    //@}
    /** @cond */
    iterator_pair GetSelection() { return GetSelection<GraphElement>(); }
    inline const_iterator_pair GetSelection() const;
    /** @endcond */

    //@{
    /**
     * @brief An iterator range returning all the nodes currently selected.
     */
    inline node_iterator_pair GetSelectionNodes();
    inline const_node_iterator_pair GetSelectionNodes() const;
    //@}

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

    //@{
    /**
     * @brief Write a text representation of the graph and all its elements
     * or a subrange of them.
     */
    virtual bool Serialise(wxOutputStream& out,
                           const iterator_pair& range = iterator_pair());
    virtual bool Serialise(Archive& archive,
                           const iterator_pair& range = iterator_pair());
    //@}

    //@{
    /**
     * @brief Load a serialised graph.
     */
    virtual bool Deserialise(wxInputStream& in);
    virtual bool Deserialise(Archive& archive);
    //@}

    //@{
    /**
     * @brief Import serialised elements into the current graph.
     */
    virtual bool DeserialiseInto(wxInputStream& in, const wxPoint& pt);
    virtual bool DeserialiseInto(Archive& archive, const wxPoint& pt);
    //@}

    //@{
    /**
     * @brief The 'snap-to-grid' flag.
     *
     * When @c true this will affect any nodes that are added or moved,
     * adjusting their positions to keep them aligned on a fix spaced grid.
     *
     * @see SetGridSpacing()
     */
    virtual void SetSnapToGrid(bool snap);
    virtual bool GetSnapToGrid() const;
    //@}

    //@{
    /**
     * @brief The grid spacing for when the 'snap-to-grid' flag is switched on.
     *
     * @tparam T @c Points or @c Twips specifying the units of @c spacing.
     * If omitted defaults to pixels.
     *
     * @see SetSnapToGrid()
     */
    template <class T> void SetGridSpacing(int spacing);
    template <class T> int GetGridSpacing() const;
    //@}
    /** @cond */
    virtual void SetGridSpacing(int spacing);
    virtual wxSize GetGridSpacing() const;
    /** @endcond */

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
    /**
     * @brief Delete the nodes and edges in the current selection.
     */
    void Clear() { Delete(GetSelection()); }

    /**
     * @brief True if the selection is non-empty. Not yet implemented.
     */
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
    /**
     * @brief True if the selection is non-empty.
     */
    virtual bool CanClear() const;

    /**
     * @brief Returns a bounding rectangle for all the elements currently
     * in the graph.
     *
     * @tparam T @c Points or @c Twips specifying the units of the returned
     * rectangle. If omitted defaults to pixels.
     */
    template <class T> wxRect GetBounds() const;
    /** @cond */
    wxRect GetBounds() const;
    /** @endcond */

    /**
     * @brief Marks the graph bounds invalid, so that they are recalculated
     * the next time GetBounds() is called.
     *
     * It is not necessary to call this directly.
     */
    void RefreshBounds();

    //@{
    /**
     * @brief The graph's parent, the handler of its events.
     *
     * The graph's event handler can be set with the @link Graph()
     * constructor@endlink.
     */
    virtual void SetEventHandler(wxEvtHandler *handler);
    virtual wxEvtHandler *GetEventHandler() const;
    //@}

    /* helper to send an event to the graph's event handler. */
    /** @cond */
    void SendEvent(wxEvent& event);
    /** @endcond */

    /** @brief The DPI of the graph's nominal pixels. */
    wxSize GetDPI() const { return m_dpi; }

    //@{
    /**
     * @brief The graph's default font.
     *
     * The font used for all elements that do not have a specific font set.
     */
    void SetFont(const wxFont& font);
    wxFont GetFont() const;
    //@}

    //@{
    /**
     * @brief Returns topmost element in the Z-order at the given coordinates.
     *
     * @tparam T If given, restricts the test to just elements of the given
     * type. If omitted defaults to @c GraphElement.
     */
    template <class T> T *HitTest(const wxPoint& pt);
    template <class T> const T *HitTest(const wxPoint& pt) const;
    //@}
    /** @cond */
    inline GraphNode *HitTest(const wxPoint& pt);
    inline const GraphNode *HitTest(const wxPoint& pt) const;
    /** @endcond */

    /**
     * @brief Render the graph onto a DC for printing or export to bitmap.
     */
    virtual void Draw(wxDC *dc, const wxRect& clip = wxRect()) const;

    /** @cond */
    wxRect GetDrawRect() const { return m_rcDraw; }
    /** @endcond */

protected:
    /**
     * @cond
     * Implementations of Add(). These not send events or delete the element
     * on failure.
     */
    virtual GraphNode *DoAdd(GraphNode *node,
                             wxPoint pt,
                             wxSize size);
    virtual GraphEdge *DoAdd(GraphNode& from,
                             GraphNode& to,
                             GraphEdge *edge = NULL);
    /** @endcond */

private:
    /** @cond */
    friend void GraphCtrl::SetGraph(Graph *graph);
    /** @endcond */

    void SetCanvas(impl::GraphCanvas *canvas);
    impl::GraphCanvas *GetCanvas() const;
    GraphCtrl *GetCtrl() const;
    wxList *GetShapeList() const;
    void DoDelete(GraphElement *element);

    static impl::GraphIteratorImpl *IterImpl(
        const wxList::iterator& begin,
        const wxList::iterator& end,
        wxClassInfo *classinfo,
        int which);

    template <class T> IterPair<T>
    Iter(int which = impl::All, wxClassInfo *classinfo = NULL) const;

    template <class T, class U> IterPair<T>
    Iter(int which = impl::All) const;

    impl::Initialisor m_initalise;
    impl::GraphDiagram *m_diagram;
    mutable wxRect m_rcBounds;
    mutable wxRect m_rcDraw;
    wxEvtHandler *m_handler;
    wxSize m_dpi;

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

template <class T>
wxPoint Graph::FindSpace(const wxSize& spacing, int columns)
{
    return FindSpace(Pixels::From<T>(spacing, m_dpi), columns);
}

template <class T>
wxPoint Graph::FindSpace(const wxPoint& position,
                         const wxSize& spacing,
                         int columns)
{
    return FindSpace(Pixels::From<T>(position, m_dpi),
                     Pixels::From<T>(spacing, m_dpi),
                     columns);
}

template <class T> void Graph::SetGridSpacing(int spacing)
{
    SetGridSpacing(Pixels::From<T>(spacing, m_dpi.y));
}

template <class T> int Graph::GetGridSpacing() const
{
    return Pixels::To<T>(GetGridSpacing().y, m_dpi.y);
}

template <class T> wxRect Graph::GetBounds() const
{
    return Pixels::To<T>(GetBounds(), GetDPI());
}

template <class T> T *Graph::HitTest(const wxPoint& pt)
{
    GraphIterator<T> begin, it;
    tie(begin, it) = GetElements<T>();

    while (it != begin)
        if ((--it)->GetBounds().Contains(pt))
            return &*it;

    return NULL;
}

template <class T> const T *Graph::HitTest(const wxPoint& pt) const
{
    GraphIterator<const T> begin, it;
    tie(begin, it) = GetElements<const T>();

    while (it != begin)
        if ((--it)->GetBounds().Contains(pt))
            return &*it;

    return NULL;
}

GraphNode *Graph::HitTest(const wxPoint& pt)
{
    return HitTest<GraphNode>(pt);
}

const GraphNode *Graph::HitTest(const wxPoint& pt) const
{
    return HitTest<const GraphNode>(pt);
}

Graph::const_iterator_pair Graph::GetElements() const
{
    return GetElements<const GraphElement>();
}

Graph::const_node_iterator_pair Graph::GetNodes() const
{
    return GetElements<const GraphNode>();
}

Graph::const_iterator_pair Graph::GetSelection() const
{
    return GetSelection<const GraphElement>();
}

Graph::node_iterator_pair Graph::GetSelectionNodes()
{
    return GetSelection<GraphNode>();
}

Graph::const_node_iterator_pair Graph::GetSelectionNodes() const
{
    return GetSelection<const GraphNode>();
}

template <class T> IterPair<T> Graph::GetSelection()
{
    return Iter<T>(impl::Selected);
}

template <class T> IterPair<const T> Graph::GetElements() const
{
    return Iter<const T>();
}

template <class T> IterPair<const T> Graph::GetNodes() const
{
    return Iter<const T, GraphNode>();
}

template <class T> IterPair<const T> Graph::GetSelection() const
{
    return Iter<const T>(impl::Selected);
}

template <class T>
IterPair<T> Graph::Iter(int which, wxClassInfo *classinfo) const
{
    wxList *list = GetShapeList();
    wxList::iterator begin, end;

    if (list) {
        begin = list->begin();
        end = list->end();
    }

    if (!classinfo)
        classinfo = CLASSINFO(T);

    return std::make_pair(
        GraphIterator<T>(IterImpl(begin, end, classinfo, which)),
        GraphIterator<T>(IterImpl(end, end, classinfo, which)));
}

template <class T, class U> IterPair<T> Graph::Iter(int which) const
{
    wxClassInfo *t = CLASSINFO(T), *u = CLASSINFO(U);

    if (t->IsKindOf(u))
        return Iter<T>(which, t);
    else if (u->IsKindOf(t))
        return Iter<T>(which, u);
    else
        return IterPair<T>();
}

/**
 * @brief Graph event
 */
class GraphEvent : public wxNotifyEvent
{
public:
    /**
     * @brief A list type used by @c #EVT_GRAPH_CONNECT and @c
     * #EVT_GRAPH_CONNECT_FEEDBACK to provide a list of all the source nodes.
     */
    typedef std::list<GraphNode*> NodeList;

    /** @brief Constructor. */
    GraphEvent(wxEventType commandType = wxEVT_NULL, int winid = 0);
    /** @brief Copy constructor. */
    GraphEvent(const GraphEvent& event);

    /** @brief Clone. */
    virtual wxEvent *Clone() const      { return new GraphEvent(*this); }

    /**
     * @brief The node being added, deleted, clicked, etc..
     */
    void SetNode(GraphNode *node)       { m_node = node; }
    /**
     * @brief Set by @c #EVT_GRAPH_CONNECT and @c #EVT_GRAPH_CONNECT_FEEDBACK
     * to indicate the target node.
     */
    void SetTarget(GraphNode *node)     { m_target = node; }
    /**
     * @brief The edge being added, deleted, clicked, etc..
     */
    void SetEdge(GraphEdge *edge)       { m_edge = edge; }

    //@{
    /**
     * @brief The cursor position for mouse related events.
     */
    void SetPosition(const wxPoint& pt) { m_pos = pt; }
    wxPoint GetPosition() const         { return m_pos; }
    //@}

    //@{
    /**
     * @brief The new size for @c #EVT_GRAPH_NODE_SIZE.
     */
    void SetSize(const wxSize& size)    { m_size = size; }
    wxSize GetSize() const              { return m_size; }
    //@}

    //@{
    /**
     * @brief A list provided by @c #EVT_GRAPH_CONNECT and
     * @c #EVT_GRAPH_CONNECT_FEEDBACK of all the source nodes.
     */
    void SetSources(NodeList& sources)  { m_sources = &sources; }
    NodeList& GetSources() const        { return *m_sources; }
    //@}

    //@{
    /**
     * @brief The new zoom percentage for @c #EVT_GRAPH_CTRL_ZOOM.
     */
    void SetZoom(double percent)        { m_zoom = percent; }
    double GetZoom() const              { return m_zoom; }
    //@}

    /**
     * @brief The node being added, deleted, clicked, etc..
     *
     * @tparam T The type of node to return. If omitted defaults to @c
     * GraphNode.
     *
     * @returns A node of the given type or NULL.
     */
    template <class T> T *GetNode() const;
    /** @cond */
    GraphNode *GetNode() const          { return m_node; }
    /** @endcond */

    /**
     * @brief Set by @c #EVT_GRAPH_CONNECT and @c #EVT_GRAPH_CONNECT_FEEDBACK
     * to indicate the target node.
     *
     * @tparam T The type of node to return. If omitted defaults to @c
     * GraphNode.
     *
     * @returns A node of the given type or NULL.
     */
    template <class T> T *GetTarget() const;
    /** @cond */
    GraphNode *GetTarget() const        { return m_target; }
    /** @endcond */

    /**
     * @brief The edge being added, deleted, clicked, etc..
     *
     * @tparam T The type of edge to return. If omitted defaults to @c
     * GraphEdge.
     *
     * @returns An edge of the given type or NULL.
     */
    template <class T> T *GetEdge() const;
    /** @cond */
    GraphEdge *GetEdge() const          { return m_edge; }
    /** @endcond */


private:
    wxPoint m_pos;
    wxSize m_size;
    GraphNode *m_node;
    GraphNode *m_target;
    GraphEdge *m_edge;
    NodeList *m_sources;
    double m_zoom;

    DECLARE_DYNAMIC_CLASS(GraphEvent)
};

// Inline definitions

template <class T> T *GraphEvent::GetNode() const
{
    return dynamic_cast<T*>(m_node);
}

template <class T> T *GraphEvent::GetTarget() const
{
    return dynamic_cast<T*>(m_target);
}

template <class T> T *GraphEvent::GetEdge() const
{
    return dynamic_cast<T*>(m_edge);
}

typedef void (wxEvtHandler::*GraphEventFunction)(GraphEvent&);

BEGIN_DECLARE_EVENT_TYPES()

    // Graph Events

    DECLARE_EVENT_TYPE(Evt_Graph_Node_Add, wxEVT_USER_FIRST + 1101)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Delete, wxEVT_USER_FIRST + 1102)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Move, wxEVT_USER_FIRST + 1103)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Size, wxEVT_USER_FIRST + 1104)

    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Add, wxEVT_USER_FIRST + 1105)
    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Delete, wxEVT_USER_FIRST + 1106)

    DECLARE_EVENT_TYPE(Evt_Graph_Connect_Feedback, wxEVT_USER_FIRST + 1107)
    DECLARE_EVENT_TYPE(Evt_Graph_Connect, wxEVT_USER_FIRST + 1108)

    // GraphCtrl Events

    DECLARE_EVENT_TYPE(Evt_Graph_Node_Click, wxEVT_USER_FIRST + 1109)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Activate, wxEVT_USER_FIRST + 1110)
    DECLARE_EVENT_TYPE(Evt_Graph_Node_Menu, wxEVT_USER_FIRST + 1111)

    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Click, wxEVT_USER_FIRST + 1112)
    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Activate, wxEVT_USER_FIRST + 1113)
    DECLARE_EVENT_TYPE(Evt_Graph_Edge_Menu, wxEVT_USER_FIRST + 1114)

    DECLARE_EVENT_TYPE(Evt_Graph_Ctrl_Zoom, wxEVT_USER_FIRST + 1115)

END_DECLARE_EVENT_TYPES()

} // namespace tt_solutions

/** @cond */
#define GraphEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction) \
        wxStaticCastEvent(tt_solutions::GraphEventFunction, &func)

#define DECLARE_GRAPH_EVT1(evt, id, fn) \
    DECLARE_EVENT_TABLE_ENTRY(tt_solutions::Evt_Graph_ ## evt, id, \
                              wxID_ANY, GraphEventHandler(fn), NULL),
#define DECLARE_GRAPH_EVT0(evt, fn) DECLARE_GRAPH_EVT1(evt, wxID_ANY, fn)
/** @endcond */

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
 * @brief Fired when a node is about to be moved.
 *
 * <code>GraphEvent::GetNode()</code> returns the node that will be moved.
 * Vetoing the event cancels the move.
 */
#define EVT_GRAPH_NODE_MOVE(fn) DECLARE_GRAPH_EVT0(Node_Move, fn)
/**
 * @brief Fired when a node's size is about to change.
 *
 * <code>GraphEvent::GetNode()</code> returns the node whose size will
 * change. Vetoing the event cancels the change.
 */
#define EVT_GRAPH_NODE_SIZE(fn) DECLARE_GRAPH_EVT0(Node_Size, fn)

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
 * @brief Handles both @c EVT_GRAPH_NODE_ADD and @c EVT_GRAPH_EDGE_ADD with
 * a single event handler.
 */
#define EVT_GRAPH_ELEMENT_ADD(fn) EVT_GRAPH_NODE_ADD(fn) EVT_GRAPH_EDGE_ADD(fn)
/**
 * @brief Handles both @c EVT_GRAPH_NODE_DELETE and @c EVT_GRAPH_EDGE_DELETE
 * with a single event handler.
 */
#define EVT_GRAPH_ELEMENT_DELETE(fn) EVT_GRAPH_NODE_DELETE(fn) EVT_GRAPH_EDGE_DELETE(fn)

/**
 * @brief Fires during node dragging each time the cursor hovers over
 * a potential target node, and allows the application to decide whether
 * dropping here would create a link.
 *
 * @c GetSources() returns a list of source nodes, and @c GetTarget() returns
 * the target node.  Removing nodes from the sources list disallows just that
 * connection while permitting other sources to connect. Vetoing the event
 * disallows all connections (it's equivalent to clearing the list).
 */
#define EVT_GRAPH_CONNECT_FEEDBACK(fn) DECLARE_GRAPH_EVT0(Connect_Feedback, fn)
/**
 * @brief This event fires when nodes have been dropped on a target node.
 *
 * @c GetSources() returns a list of source nodes, and @c GetTarget() returns
 * the target node.  Removing a node from the sources list disallows just that
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
 * @brief Fires when the control zoom factor changes.
 */
#define EVT_GRAPH_CTRL_ZOOM(id, fn) DECLARE_GRAPH_EVT1(Ctrl_Zoom, id, fn)

/**
 * @brief Handles both @c EVT_GRAPH_NODE_CLICK and @c EVT_GRAPH_EDGE_CLICK
 * with a single event handler.
 */
#define EVT_GRAPH_ELEMENT_CLICK(id, fn) EVT_GRAPH_NODE_CLICK(id, fn) EVT_GRAPH_EDGE_CLICK(id, fn)
/**
 * @brief Handles both @c EVT_GRAPH_NODE_ACTIVATE and @c
 * EVT_GRAPH_EDGE_ACTIVATE with a single event handler.
 */
#define EVT_GRAPH_ELEMENT_ACTIVATE(id, fn) EVT_GRAPH_NODE_ACTIVATE(id, fn) EVT_GRAPH_EDGE_ACTIVATE(id, fn)
/**
 * @brief Handles both @c EVT_GRAPH_NODE_MENU and @c EVT_GRAPH_EDGE_MENU with
 * a single event handler.
 */
#define EVT_GRAPH_ELEMENT_MENU(id, fn) EVT_GRAPH_NODE_MENU(id, fn) EVT_GRAPH_EDGE_MENU(id, fn)

#endif // GRAPHCTRL_H
