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

// These definitions are also present in wx/ogl/defs.h, but we don't want to
// depend on that header here, so reproduce them.
#ifndef wxHAS_INFO_DC
    using wxReadOnlyDC = wxDC;
    using wxInfoDC = wxClientDC;
#endif

#include <iterator>
#include <list>
#include <memory>

#include "factory.h"
#include "archive.h"
#include "coords.h"
#include "iterrange.h"

/**
 * @file graphctrl.h
 * @brief Header for the graph control GUI component.
 */

class wxShape;
class wxLineShape;

/**
 * @brief TT-Solutions
 */
namespace tt_solutions {

constexpr double DEFAULT_VERT_SPACING_IN_INCHES = 0.5;
constexpr double DEFAULT_HORZ_SPACING_IN_INCHES = 0.3;

/// Shape representation used in the underlying graphics library.
typedef wxShape GraphShape;

/// Line representation used in the underlying graphics library.
typedef wxLineShape GraphLineShape;

class Graph;
class GraphElement;
class GraphNode;
class TipWindow;

/*
 * Implementation classes
 */
namespace impl
{
    class GraphIteratorImpl;
    class GraphDiagram;
    class GraphCanvas;

    /**
     * @brief Conditions allowing to filter the elements being iterated on.
     *
     * This is not used in the public API but only by Graph::IterImpl() and
     * related methods.
     */
    enum IteratorFilter
    {
        All,            ///< All elements are included.
        Selected,       ///< Only selected elements are included.
        InEdges,        ///< Only incoming (to this node) edges are included.
        OutEdges        ///< Only outgoing (from this node) edges are included.
    };

    /**
     * Base class for different kinds of iterators over graph elements.
     *
     * This class is a standard-like iterator, in particular it provides the
     * same member typedefs as the standard iterators.
     *
     * It is implemented using "pImpl" idiom so the real iteration logic is in
     * the internal GraphIteratorImpl class.
     */
    class GraphIteratorBase
    {
    public:
        /// Iterator category marks this iterator as being bidirectional.
        typedef std::bidirectional_iterator_tag iterator_category;
        /// Type of the elements being iterated over.
        typedef GraphElement value_type;
        /// Type for the distance between two iterators.
        typedef ptrdiff_t difference_type;
        /// Pointer to the elements being iterated over.
        typedef GraphElement* pointer;
        /// Reference to the elements being iterated over.
        typedef GraphElement& reference;

        /// Default constructor creates iterator in an invalid state.
        GraphIteratorBase();

        /// Copy constructor.
        GraphIteratorBase(const GraphIteratorBase& it);

        /// Move constructor.
        GraphIteratorBase(GraphIteratorBase&& it) noexcept;

        /// Constructor from the internal implementation object.
        GraphIteratorBase(GraphIteratorImpl *impl);

        ~GraphIteratorBase();

        /// Dereference an iterator. Must be valid.
        GraphElement& operator*() const;

        /// Dereference an iterator. Must be valid.
        GraphElement* operator->() const {
            return &**this;
        }

        /// Assignment operator from another iterator.
        GraphIteratorBase& operator=(const GraphIteratorBase& it);

        /// Move assignment operator from another iterator.
        GraphIteratorBase& operator=(GraphIteratorBase&& it) noexcept;

        /// Advance the iterator, in prefix and postfix forms.
        //@{
        GraphIteratorBase& operator++();

        GraphIteratorBase operator++(int) {
            GraphIteratorBase it(*this);
            ++(*this);
            return it;
        }
        //@}

        /// Advance the iterator in reverse direction, prefix and postfix.
        //@{
        GraphIteratorBase& operator--();

        GraphIteratorBase operator--(int) {
            GraphIteratorBase it(*this);
            --(*this);
            return it;
        }
        //@}

        /// Compare iterator with another one.
        //@{
        bool operator==(const GraphIteratorBase& it) const;

        bool operator!=(const GraphIteratorBase& it) const {
            return !(*this == it);
        }
        //@}

    private:
        /// The implementation object owned by the iterator.
        std::unique_ptr<GraphIteratorImpl> m_impl;
    };

    /**
     * Class used to initialize OGL library only once.
     */
    class Initialisor
    {
    public:
        Initialisor();
        ~Initialisor();

    private:
        /// Forbid copy construction for a singleton class.
        Initialisor(const Initialisor&);

        /// The initialization counter.
        static int m_initalise;
    };

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
 * The graph methods that return iterators take a template parameter to allow
 * the type of the iterator to be specified. For example @c
 * GetNodes<ProjectNode>() would return @c GraphIterator<ProjectNode>
 * iterators.
 *
 * Methods that return iterators return a begin/end pair in a
 * <code>std::pair</code> from which an iterable range can be constructed
 * using the '<code>MakeRange()</code>' function, so the usual idiom for using
 * them is:
 *
 * @code
 *  for (auto& node : MakeRange(m_graph->GetSelection<ProjectNode>()))
 *      node.SetSize(size);
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
 * MakeRange \n
 * Graph::GetNodes() \n
 * Graph::GetSelection() \n
 * GraphNode::GetEdges() \n
 * GraphEdge::GetNodes()
 */
template <class T>
class GraphIterator : public impl::GraphIteratorBase
{
public:
    /// Iterator category marks this iterator as being bidirectional.
    typedef std::bidirectional_iterator_tag iterator_category;
    /// Type of the elements being iterated over.
    typedef T value_type;
    /// Type for the distance between two iterators.
    typedef ptrdiff_t difference_type;
    /// Pointer to the elements being iterated over.
    typedef T* pointer;
    /// Reference to the elements being iterated over.
    typedef T& reference;

    /// Synonym for a pair of iterators.
    typedef typename std::pair<GraphIterator, GraphIterator> pair;


    /// Default ctor.
    GraphIterator() : Base() { }

    /// Copy ctor.
    GraphIterator(const GraphIterator& it) : Base(it) { }

    /// Template copy ctor.
    template <class U>
    GraphIterator(const GraphIterator<U>& it) : Base(it) {
        U *u = 0;
        CheckAssignable(u);
    }

    /// Ctor from the internal implementation object.
    GraphIterator(impl::GraphIteratorImpl *impl) : Base(impl) { }

    ~GraphIterator() { }

    /// Dereference an iterator. Must be valid.
    T& operator*() const {
        return static_cast<T&>(Base::operator*());
    }

    /// Dereference an iterator. Must be valid.
    T* operator->() const {
        return &**this;
    }

    /// Assignment operator.
    GraphIterator& operator=(const GraphIterator& it) {
        Base::operator=(it);
        return *this;
    }

    /// Advance the iterator, in prefix and postfix forms.
    //@{
    GraphIterator& operator++() {
        Base::operator++();
        return *this;
    }

    GraphIterator operator++(int) {
        GraphIterator it(*this);
        ++(*this);
        return it;
    }
    //@}

    /// Advance the iterator in reverse direction, prefix and postfix.
    //@{
    GraphIterator& operator--() {
        Base::operator--();
        return *this;
    }

    GraphIterator operator--(int) {
        GraphIterator it(*this);
        --(*this);
        return it;
    }
    //@}

    /// Compare iterator with another one.
    //@{
    bool operator==(const GraphIterator& it) const {
        return Base::operator==(it);
    }

    bool operator!=(const GraphIterator& it) const {
        return !(*this == it);
    }
    //@}

private:
    /// Base, non-template, iterator class.
    typedef impl::GraphIteratorBase Base;

    /**
     * Function used for compile-time type compatibility check.
     *
     * This function is used in the template copy ctor to ensure that the type
     * of the iterator passed to it is compatible with the type we use: if it
     * isn't, calling this function with an object of type @c U* would fail.
     */
    void CheckAssignable(T*) { }
};

/**
 * @brief A shorter synonym for <code>std::pair< GraphIterator<T>,
 * GraphIterator<T> ></code>.
 */
template <class T>
using IterPair = std::pair< GraphIterator<T>, GraphIterator<T> >;

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
    /**
     * Select or deselect this element.
     *
     * Used to implement the public Select() and Unselect() functions.
     */
    virtual void DoSelect(bool select);

    /**
     * Update the shape after the change of some of its attributes.
     */
    virtual void UpdateShape() = 0;

    /**
     * Set the shape of this element.
     *
     * We take ownership of the passed in pointer.
     */
    virtual void SetShape(GraphShape *shape);

    /**
     * Return the associated shape.
     *
     * @see GetShape()
     */
    virtual GraphShape *DoGetShape() const { return m_shape; }

    /**
     * Ensure that we have the associated shape and return it.
     *
     * @see EnsureShape()
     */
    virtual GraphShape *DoEnsureShape();

    /** @brief The DPI of the graph's nominal pixels. */
    virtual wxSize GetDPI() const;

private:
    impl::Initialisor m_initalise;          ///< Initialization counter.

    wxColour m_colour;                      ///< Main element colour.
    wxColour m_bgcolour;                    ///< Background element colour.

    int m_style;                            ///< Element style. @see Style.
    GraphShape *m_shape;                    ///< The underlying shape.

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
    void SetStyle(int style) override;

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
    bool Serialise(Archive::Item& arc) override;

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
    void SetEdgeShape(GraphLineShape *shape);

    /**
     * @brief This is called by methods that affect the appearance of the
     * element.
     *
     * Such as SetSize(), SetText() and SetFont(), to give the element the
     * chance to adjust its layout. The default implementation does nothing.
     */
    void Layout() override { }

    /** @brief Overridable returning the pen that will be used. */
    wxPen GetPen() const override { return wxPen(GetColour(), m_linewidth); }

protected:
    void UpdateShape() override { }

    /**
     * Move this edge to the front of the list of shapes.
     *
     * This is used by Serialise() when extracting from the archive.
     */
    bool MoveFront();

private:
    /**
     * @brief Return iterator for one the edge connection points.
     *
     * If @a end is false, returns the starting point, otherwise the end one.
     */
    impl::GraphIteratorImpl *IterImpl(
        GraphLineShape *line, wxClassInfo *classinfo, bool end) const;

    /**
     * @brief Returns the nodes connected by this edge.
     *
     * The first component of the returned pair corresponds to the starting
     * node and the second one -- to the ending one.
     */
    template <class T> IterPair<T> Iter() const;

    int m_arrowsize;        ///< Size of the arrow head, if any. Default is 10.
    int m_linewidth;        ///< Width of the line in pixels. Default is 1.

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
    void SetStyle(int style) override;

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
    bool Serialise(Archive::Item& arc) override;

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
    void SetShape(wxShape *shape) override;

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
    void DoSelect(bool select) override;
    void UpdateShape() override;

    /**
     * Ensure that we use the correct text colour.
     *
     * Should be called after changing m_textcolour.
     */
    virtual void UpdateShapeTextColour();

    /**
     * Implementation of SetSize().
     *
     * This is called only if the resizing does happen, i.e. wasn't vetoed by
     * an event handler.
     */
    virtual void DoSetSize(wxReadOnlyDC& dc, const wxSize& size);

    /**
     * @brief Overridable called from Layout().
     */
    virtual void OnLayout(wxReadOnlyDC&) { }
    /**
     * @brief Calculates the positions of any text labels, icons, etc.
     * within the node.
     *
     * This is called by methods that affect the appearance of the node, such
     * as SetSize(), SetText() and SetFont(), to give the node the chance
     * to adjust its layout. It calls OnLayout() to allow derived classes
     * to take care of any custom features they add.
     */
    void Layout() override;

private:
    /// Get the list of all underlying lines connecting to this node.
    wxList *GetLines() const;

    /**
     * @brief Return iterator for all edges connecting to this node.
     *
     * This is used by Iter() below.
     */
    impl::GraphIteratorImpl *IterImpl(
        const wxList::iterator& begin,
        const wxList::iterator& end,
        wxClassInfo *classinfo,
        int which) const;

    /**
     * @brief Return a range of iterators over all edges connecting to this
     * node.
     *
     * @a which here should be either impl::InEdges or impl::OutEdges (or
     * impl::All for both).
     *
     * This is used to implement the public GetEdges(), GetInEdges() and
     * GetOutEdges() methods.
     */
    template <class T> IterPair<T> Iter(int which = impl::All) const;

    wxColour m_textcolour;      ///< Colour of the node text.
    wxString m_text;            ///< Node text content.
    wxString m_tooltip;         ///< Tooltip shown for the node.
    wxString m_rank;            ///< Node rank for layout.
    wxFont m_font;              ///< Font used to render the node text.

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
              long style = wxBORDER,
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
     *
     * @param side Combination of wxLEFT or wxRIGHT and wxTOP or wxBOTTOM.
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
    void Fit() override;

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

    /**
     * @brief Enable/Disable the tooltips.
     *
     * @see EnableToolTips() \n SetToolTipDelay()
     */
    enum ToolTipMode {
        Tip_Disable,        /**< Disable tooltips. */
        Tip_Enable,         /**< Enable tooltips using custom implementation. */
        Tip_wxRichToolTip,  /**< Enable using wxRichToolTip. */
        Tip_wxToolTip       /**< Enable using wxToolTip. */
    };

    //@{
    /**
     * @brief Enable/Disable the tooltips.
     *
     * Setting this to @c Tip_Enable enables the new @c TipWindow tooltips.
     *
     * Setting it to @c Tip_wxToolTip instead uses the tooltip code
     * from the previous version. This is known <em>not</em> to work on
     * some versions of GTK+. It's provided as a backup for Windows builds
     * where it has had more testing than the new code.
     *
     * When enabled here, the tooltip are still disabled by setting the tip
     * delay to zero (see @c SetToolTipDelay()) or by calling @c
     * wxToolTip::Enable(false) (in the case of @c Tip_wxToolTip).
     */
    void EnableToolTips(ToolTipMode mode) { m_tipmode = mode; }
    int ToolTipsEnabled() const { return m_tipmode; }
    //@}

    //@{
    /**
     * @brief The delay in milliseconds before nodes' tooltips are shown.
     *
     * Setting this to zero will disable the tooltips.
     * When the tooltip mode is @c Tip_wxToolTip the delay must be set
     * with @c wxToolTip::SetDelay().
     *
     * @see SetToolTipMode()
     */
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

    /**
     * Size event handler.
     *
     * Notify the canvas about the new size.
     */
    void OnSize(wxSizeEvent& event);

    /**
     * Key press event handler.
     *
     * Used to implement scrolling using the cursor keys.
     */
    void OnChar(wxKeyEvent& event);

    /**
     * Mouse wheel event handler.
     *
     * Used to implement scrolling using the mouse wheel.
     */
    void OnMouseWheel(wxMouseEvent& event);

    /**
     * Mouse move event handler.
     *
     * Used to show tooltips and handle dragging of the graph elements.
     */
    void OnMouseMove(wxMouseEvent& event);

    /**
     * Mouse leave event handler.
     *
     * Used to hide the tooltip when the mouse leaves the window.
     */
    void OnMouseLeave(wxMouseEvent& event);

    /**
     * Timer event handler for the tooltip timer.
     *
     * Used to show a tooltip when the mouse remains over a node for a
     * sufficiently long time.
     */
    void OnTipTimer(wxTimerEvent& event);

    /**
     * Idle event handler.
     *
     * Used for scrollbar and cursor updating.
     */
    void OnIdle(wxIdleEvent& event);

#ifdef __WXGTK__
    /**
     * wxGTK-only focus event handlers.
     *
     * Under wxGTK we need to explicitly hide the tooltip when we lose focus
     * and possibly show it when we gain it.
     */
    //@{
    void OnSetFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    //@}
#endif

    /** Default value for the constructor's name parameter. */
    static const wxChar DefaultName[];

protected:
    /** Returns the DPI used by the control. */
    wxSize GetDPI() const override;

private:
    /**
     * @brief Show or hide the tooltip at the given position.
     *
     * This function is called to update the state of the tooltip. It is done
     * on mouse move, when we get focus and also from idle time.
     *
     * It may call either OpenTip() or CloseTip().
     */
    void CheckTip(const wxPoint& pt = wxGetMousePosition());

    /**
     * @brief Do show the given tip.
     *
     * Called by CheckTip() only when really needed, i.e. if @a node is under
     * mouse and has a non-empty tooltip.
     */
    void OpenTip(const GraphNode& node);

    /**
     * @brief Remove the currently shown tip.
     *
     * Called by CheckTip().
     */
    void CloseTip(const wxPoint& pt = wxDefaultPosition);

    impl::Initialisor m_initalise;  ///< Initialization counter.
    impl::GraphCanvas *m_canvas;    ///< The associated canvas.
    Graph *m_graph;                 ///< The associated graph object.

    /**
        @name Tooltip data.

        Note that m_tipopen can be true even when m_tipwin is still null, as
        the tip window is shown after a delay.
     */
    //@{
    wxTimer m_tiptimer;             ///< Timer used in Tip_Enable mode.
    ToolTipMode m_tipmode;          ///< See ToolTipMode elements.
    int m_tipdelay;                 ///< Tooltip delay in Tip_Enable mode.
    GraphNode *m_tipnode;           ///< Node for which tooltip is shown.
    TipWindow *m_tipwin;            ///< Tooltip window or NULL.
    bool m_tipopen;                 ///< True if a tooltip is currently shown,
    //@}

    static int sm_leftDrag;         ///< DragMode value for left mouse button.
    static int sm_rightDrag;        ///< DragMode value for right mouse button.

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
                           double ranksep = DEFAULT_VERT_SPACING_IN_INCHES,
                           double nodesep = DEFAULT_HORZ_SPACING_IN_INCHES);
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
                        double ranksep = DEFAULT_VERT_SPACING_IN_INCHES,
                        double nodesep = DEFAULT_HORZ_SPACING_IN_INCHES);

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

    /** Helper to send an event to the graph's event handler. */
    void SendEvent(wxEvent& event);

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
     * @brief Returns topmost node in the Z-order at the given coordinates.
     *
     * @tparam T If given, restricts the test to just elements of the given
     * type. If omitted defaults to @c GraphNode.
     */
    template <class T> T *HitTest(const wxPoint& pt);
    template <class T> const T *HitTest(const wxPoint& pt) const;
    //@}
    /** @cond */
    GraphNode *HitTest(const wxPoint& pt);
    const GraphNode *HitTest(const wxPoint& pt) const;
    /** @endcond */

    /**
     * @brief Render the graph onto a DC for printing or export to bitmap.
     */
    virtual void Draw(wxDC *dc, const wxRect& clip = wxRect()) const;

    /**
     * Return the temporary clipping region.
     *
     * This method is used by the implementation only.
     *
     * @see m_rcDraw
     */
    wxRect GetDrawRect() const { return m_rcDraw; }

protected:
    /**
     * Implementations of Add().
     * These methods don't send events nor delete the element on failure.
     */
    //@{
    virtual GraphNode *DoAdd(GraphNode *node,
                             wxPoint pt,
                             wxSize size);
    virtual GraphEdge *DoAdd(GraphNode& from,
                             GraphNode& to,
                             GraphEdge *edge = NULL);
    //@}

private:
    /** @cond */
    friend void GraphCtrl::SetGraph(Graph *graph);
    /** @endcond */

    /// Set the canvas used for the graph display.
    void SetCanvas(impl::GraphCanvas *canvas);

    /// Get the canvas used for the graph display. Never @c NULL.
    impl::GraphCanvas *GetCanvas() const;

    /// Get the associated control.
    GraphCtrl *GetCtrl() const;

    /// Get the list of all shapes in the graph.
    wxList *GetShapeList() const;

    /// Delete an element from the graph.
    void DoDelete(GraphElement *element);

    /**
     * @brief Creates a new iterator over graph elements.
     *
     * The returned iterator starts at @a begin and goes until @a end. It will
     * only return the elements of the given @a classinfo (i.e. skip all the
     * rest) and also filter them by IteratorFilter enum elements.
     */
    static impl::GraphIteratorImpl *IterImpl(
        const wxList::iterator& begin,
        const wxList::iterator& end,
        wxClassInfo *classinfo,
        int which);

    /**
     * @brief Return a pair of iterators defining a range with all elements of
     * the given type.
     *
     * This method returns the begin and end iterators defining a range of all
     * elements satisfying the given condition @a which and of the specified
     * type @a classinfo if it is non-NULL or of type @c T by default.
     *
     * @see IterImpl()
     */
    template <class T> IterPair<T>
    Iter(int which = impl::All, wxClassInfo *classinfo = NULL) const;

    /**
     * @brief Return a pair of iterators defining a range with all elements of
     * the common base type.
     *
     * This method returns all elements of type @c T if @c U derives from @c T
     * or of type @c U if @c T derives from @c U. If neither is true, an empty
     * range is returned.
     */
    template <class T, class U> IterPair<T>
    Iter(int which = impl::All) const;

    impl::Initialisor m_initalise;          ///< Initialization counter.
    impl::GraphDiagram *m_diagram;          ///< Associated OGL diagram.

    /**
     * @brief Bounding box coordinates.
     *
     * This is computed on demand, use GetBounds() to access it and
     * RefreshBounds() to invalidate.
     */
    mutable wxRect m_rcBounds;

    /**
     * @brief Temporary clipping region.
     *
     * This is used to set the clipping region when redrawing the diagram.
     *
     * @see Draw(), GetDrawRect()
     */
    mutable wxRect m_rcDraw;

    /**
     * @brief Current hit testing region.
     *
     * Used only in HitTest() to optimize consecutive calls to it for close
     * locations.
     */
    mutable wxRect m_rcHit;

    /**
     * @brief Last node returned by HitTest().
     *
     * This is another optimization used in HitTest().
     */
    mutable const GraphNode *m_nodeHit;

    /**
     * @brief Event handler used for generation of all the events.
     *
     * This event handler is used as a sink for all the events generated by
     * this class if it is non-NULL.
     *
     * @see SetEventHandler(), GetEventHandler()
     */
    wxEvtHandler *m_handler;

    /**
     * @brief Screen resolution in dots per inches.
     *
     * Set in ctor and used for coordinate units transformations.
     */
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
    return dynamic_cast<T*>(HitTest(pt));
}

template <class T> const T *Graph::HitTest(const wxPoint& pt) const
{
    return dynamic_cast<T*>(HitTest(pt));
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
     * @brief A list type used by @c EVT_GRAPH_CONNECT and @c
     * EVT_GRAPH_CONNECT_FEEDBACK to provide a list of all the source nodes.
     */
    typedef std::list<GraphNode*> NodeList;

    /** @brief Constructor. */
    GraphEvent(wxEventType commandType = wxEVT_NULL, int winid = 0);
    /** @brief Copy constructor. */
    GraphEvent(const GraphEvent& event) = default;

    /** @brief Clone. */
    wxEvent *Clone() const override { return new GraphEvent(*this); }

    /**
     * @brief The node being added, deleted, clicked, etc.
     */
    void SetNode(GraphNode *node)       { m_node = node; }
    /**
     * @brief Set by @c EVT_GRAPH_CONNECT and @c EVT_GRAPH_CONNECT_FEEDBACK
     * to indicate the target node.
     */
    void SetTarget(GraphNode *node)     { m_target = node; }
    /**
     * @brief The edge being added, deleted, clicked, etc.
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
     * @brief The new size for @c EVT_GRAPH_NODE_SIZE.
     */
    void SetSize(const wxSize& size)    { m_size = size; }
    wxSize GetSize() const              { return m_size; }
    //@}

    //@{
    /**
     * @brief A list provided by @c EVT_GRAPH_CONNECT and
     * @c EVT_GRAPH_CONNECT_FEEDBACK of all the source nodes.
     */
    void SetSources(NodeList& sources)  { m_sources = &sources; }
    NodeList& GetSources() const        { return *m_sources; }
    //@}

    //@{
    /**
     * @brief The new zoom percentage for @c EVT_GRAPH_CTRL_ZOOM.
     */
    void SetZoom(double percent)        { m_zoom = percent; }
    double GetZoom() const              { return m_zoom; }
    //@}

    /**
     * @brief The node being added, deleted, clicked, etc.
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
     * @brief Set by @c EVT_GRAPH_CONNECT and @c EVT_GRAPH_CONNECT_FEEDBACK
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
     * @brief The edge being added, deleted, clicked, etc.
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
    wxPoint m_pos;          ///< Position of the event.
    wxSize m_size;          ///< New size for resize event.
    GraphNode *m_node;      ///< The node associated with the event.
    GraphNode *m_target;    ///< Target for connection events.
    GraphEdge *m_edge;      ///< The edge being added, deleted &c.
    NodeList *m_sources;    ///< Source nodes for connection events.
    double m_zoom;          ///< New zoom factor for zoom events.

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

/**
 * @brief Type of the handler for GraphEvent events.
 *
 * All handlers for graph events must have this signature. As usual, the event
 * macros will check for it.
 */
typedef void (wxEvtHandler::*GraphEventFunction)(GraphEvent&);

BEGIN_DECLARE_EVENT_TYPES()
    /** @cond */
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

    DECLARE_EVENT_TYPE(Evt_Graph_Click, wxEVT_USER_FIRST + 1115)
    DECLARE_EVENT_TYPE(Evt_Graph_Menu, wxEVT_USER_FIRST + 1116)

    DECLARE_EVENT_TYPE(Evt_Graph_Ctrl_Zoom, wxEVT_USER_FIRST + 1117)
    /** @endcond */
END_DECLARE_EVENT_TYPES()

/**
 * @brief Helper macro for use with Connect().
 *
 * When using wxEvtHandler::Connect() to connect to the graph events
 * dynamically, this macro should be applied to the event handler.
 */
#define GraphEventHandler(func) \
    wxEVENT_HANDLER_CAST(tt_solutions::GraphEventFunction, func)

/** @cond */
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
 * @brief Fires when the graph background is clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_CLICK(id, fn) DECLARE_GRAPH_EVT1(Click, id, fn)
/**
 * @brief Fires when the graph background is right clicked.
 *
 * <code>GraphEvent::GetPosition()</code> returns the position of the
 * mouse click.
 */
#define EVT_GRAPH_MENU(id, fn) DECLARE_GRAPH_EVT1(Menu, id, fn)

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

} // namespace tt_solutions

#endif // GRAPHCTRL_H
