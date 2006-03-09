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
#include "wx/wx.h"

namespace tt_solutions {

class Graph;
class GraphElement;
class GraphNode;

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

class GraphElement : public wxObject
{
public:
    GraphElement() { }
    virtual ~GraphElement() { }

    virtual wxColour GetColour() const = 0;
    virtual wxColour GetBackgroundColour() const = 0;

    virtual GraphElement *SetColor(const wxColour& colour) = 0;
    virtual GraphElement *SetBackgroundColour(const wxColour& colour) = 0;

    virtual GraphElement *Select() = 0;
    virtual GraphElement *UnSelect() = 0;
    virtual bool IsSelected() = 0;
 
    virtual bool Serialize(wxOutputStream& out) = 0;
    virtual bool Deserialize(wxInputStream& in) = 0;
};

class GraphEdge : public GraphElement
{
public:
    typedef GraphIterator<GraphNode> iterator;
    typedef GraphIterator<const GraphNode> const_iterator;

    enum { style_line, style_arrow };

    GraphEdge();
    ~GraphEdge();

    int GetStyle() const                 { return m_style; }
    wxColour GetColour() const           { return m_colour; }
    wxColour GetBackgroundColour() const { return m_bgcolour; }
    bool IsSelected()                    { return m_selected; }

    GraphEdge *SetStyle();
    GraphEdge *SetColor(const wxColour& colour);
    GraphEdge *SetBackgroundColour(const wxColour& colour);
    GraphEdge *Select();
    GraphEdge *UnSelect();
 
    std::pair<iterator, iterator> GetNodes();
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

class GraphNode : public GraphElement
{
public:
    typedef GraphIterator<GraphEdge> iterator;
    typedef GraphIterator<const GraphEdge> const_iterator;

    enum { style_rectangle };
    enum { hittest_image = 1, hittest_text };

    GraphNode();
    ~GraphNode();
 
    wxString GetText() const             { return m_text; }
    wxFont GetFont() const               { return m_font; }
    int GetStyle() const                 { return m_style; }
    wxColour GetColour() const           { return m_colour; }
    wxColour GetTextColour() const       { return m_textcolour; }
    wxColour GetBackgroundColour() const { return m_bgcolour; }
    bool IsSelected()                    { return m_selected; }

    GraphNode *SetText(const wxString& text);
    GraphNode *SetFont(const wxFont& font);
    GraphNode *SetStyle();
    GraphNode *SetColor(const wxColour& colour);
    GraphNode *SetTextColor(const wxColour& colour);
    GraphNode *SetBackgroundColour(const wxColour& colour);
    GraphNode *Select();
    GraphNode *UnSelect();

    void EnsureVisible();
    void ScrollTo();

    int HitTest(wxPoint pt) const;

    std::pair<iterator, iterator> GetEdges();
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

class GraphCtrl : public wxScrolledWindow
{
public:
    GraphCtrl(wxWindow *parent,
              wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxBORDER | wxRETAINED,
              const wxString& name = DefaultName);
    ~GraphCtrl();

    void SetZoom(int percent);
    int GetZoom();

    void SetGraph(Graph *graph);
    Graph *GetGraph() const { return m_graph; }

    void OnSize(wxSizeEvent& event);

    static const wxChar DefaultName[];

private:
    implementation::GraphCanvas *m_canvas;
    Graph *m_graph;

    DECLARE_EVENT_TABLE()
};

class Graph : public wxEvtHandler
{
public:
    typedef GraphIterator<GraphElement> iterator;
    typedef GraphIterator<GraphNode> node_iterator;

    typedef GraphIterator<const GraphElement> const_iterator;
    typedef GraphIterator<const GraphNode> const_node_iterator;

    typedef std::pair<iterator, iterator> iterator_pair;
    typedef std::pair<const_iterator, const_iterator> const_iterator_pair;

    Graph();
    ~Graph();

    void Add(GraphNode *node, wxPoint pt = wxDefaultPosition);
    void Add(const GraphNode& from, const GraphNode& to, GraphEdge *edge = NULL);

    void Delete(GraphElement *element);
    void Delete(iterator_pair range);
    
    bool Layout();
    bool Layout(iterator_pair range);

    void Select(GraphElement *element);
    void Select(iterator_pair range);
    
    void Unselect(GraphElement *element);
    void Unselect(iterator_pair range);

    std::pair<node_iterator, node_iterator> GetNodes();
    iterator_pair GetElements();
    iterator_pair GetSelected();

    std::pair<const_node_iterator, const_node_iterator> GetNodes() const;
    const_iterator_pair GetElements() const;
    const_iterator_pair GetSelected() const;

    bool Serialize(wxOutputStream& out) const;
    bool Serialize(wxOutputStream& out, const_iterator_pair range) const;
    bool Deserialize(wxInputStream& in);

    void SetSnapToGrid(bool snap);
    bool GetSnapToGrid() const;
    void SetGridSpacing(int spacing);
    int GetGridSpacing() const;
    
    void Undo();
    void Redo();

    bool CanUndo() const;
    bool CanRedo() const;

    bool Cut();
    bool Copy();
    bool Paste();
    void Clear();

    bool CanCut() const;
    bool CanCopy() const;
    bool CanPaste() const;
    bool CanClear() const;

private:
    friend void GraphCtrl::SetGraph(Graph *graph);

    implementation::GraphDiagram *m_diagram;

    DECLARE_EVENT_TABLE()
};

class GraphEvent : public wxNotifyEvent
{
public:
    GraphEvent(wxEventType commandType = wxEVT_NULL, int id = 0);
    GraphEvent(const GraphEvent& event);

    virtual wxEvent *Clone() const { return new GraphEvent(*this); }

    GraphEvent *SetNode(GraphNode *node)    { m_node = node; return this; }
    GraphEvent *SetTarget(GraphNode *node)  { m_target = node; return this; }
    GraphEvent *SetEdge(GraphEdge *edge)    { m_edge = edge; return this; }
    GraphEvent *SetPoint(const wxPoint& pt) { m_point = pt; return this; }

    GraphNode *GetNode() const              { return m_node; }
    GraphNode *GetTarget() const            { return m_target; }
    GraphEdge *GetEdge() const              { return m_edge; }
    wxPoint GetPoint() const                { return m_point; }

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
