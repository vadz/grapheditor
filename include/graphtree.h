/////////////////////////////////////////////////////////////////////////////
// Name:        graphtree.h
// Purpose:     Tree control drag source for graph nodes
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef GRAPHTREE_H
#define GRAPHTREE_H

/**
 * @file graphtree.h
 * @brief Tree control drag source for graph nodes
 */

#include "graphctrl.h"
#include <wx/treectrl.h>
#include <wx/dragimag.h>

namespace tt_solutions {

/**
 * @brief Tree control with items that can be dragged onto a GraphCtrl
 * to create new nodes.
 *
 * Dropping a node fires the event <code>#EVT_GRAPHTREE_DROP</code>.
 *
 * @see GraphTreeEvent
 */
class GraphTreeCtrl : public wxTreeCtrl
{
public:
    /// Default ctor.
    GraphTreeCtrl() { Init(); }

    /// Constructor taking the same arguments as wxTreeCtrl.
    GraphTreeCtrl(wxWindow *parent, wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxTR_HAS_BUTTONS | wxTR_NO_LINES | wxTR_HIDE_ROOT,
               const wxValidator &validator = wxDefaultValidator,
               const wxString& name = DefaultName)
        : wxTreeCtrl(parent, id, pos, size, style, validator, name)
    {
        Init();
    }

#ifdef wxHAS_GENERIC_TREECTRL
    /**
     * @brief Override base class function to avoid auto scrolling.
     *
     * This interferes with drag and drop.
     */
    bool SendAutoScrollEvents(wxScrollWinEvent&) const override { return false; }
#endif // wxHAS_GENERIC_TREECTRL

    /// Event handler for begin drag event.

    void OnBeginDrag(wxTreeEvent& event);

    /// Event handler for dragging events.
    void OnMouseMove(wxMouseEvent& event);

    /// Event handler for dragging end event.
    void OnLeftButtonUp(wxMouseEvent& event);

    /// Default name for GraphTreeCtrl objects.
    static const wxChar DefaultName[];

private:
    /// Common part of all ctors.
    void Init() { m_dragImg = NULL; }

    wxDragImage *m_dragImg;     ///< Drag image for item being dragged.
    wxTreeItemId m_dragItem;    ///< The item being currently dragged.

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(GraphTreeCtrl)
    DECLARE_NO_COPY_CLASS(GraphTreeCtrl)
};

/**
 * @brief Event that fires when an item from the GraphTreeCtrl is dropped
 * on a GraphCtrl.
 *
 * For example:
 * @code
 *  void MyFrame::OnGraphTreeDrop(GraphTreeEvent& event)
 *  {
 *      ProjectNode *node = new ProjectNode;
 *      node->SetText(event.GetString());
 *      node->SetResult(_T("this is a multi-\nline test"));
 *      node->SetColour(0x16a8fa);
 *      node->SetIcon(event.GetIcon());
 *      m_graph->Add(node, event.GetPosition());
 *  }
 * @endcode
 *
 * @see EVT_GRAPHTREE_DROP
 */
class GraphTreeEvent : public wxCommandEvent
{
public:
    /** @brief Constructor. */
    GraphTreeEvent(wxEventType commandType = wxEVT_NULL, int winid = 0)
        : wxCommandEvent(commandType, winid)
    { }

    /** @brief Copy constructor. */
    GraphTreeEvent(const GraphTreeEvent& event) = default;

    /** @brief The GraphCtrl that is the target of a drop. */
    GraphCtrl *GetTarget() const { return m_target; }
    /** @brief The GraphCtrl that is the target of a drop. */
    void SetTarget(GraphCtrl *target) { m_target = target; }

    /** @brief The position in Graph coordinates of a drop. */
    wxPoint GetPosition() const { return m_pos; }
    /** @brief The position in Graph coordinates of a drop. */
    void SetPosition(const wxPoint& pt) { m_pos = pt; }

    /** @brief The tree control item id for the item dropped. */
    wxTreeItemId GetItem() const { return m_item; }
    /** @brief The tree control item id for the item dropped. */
    void SetItem(const wxTreeItemId& item) { m_item = item; }

    /** @brief The image of the item dropped. */
    wxIcon GetIcon() const { return m_icon; }
    /** @brief The image of the item dropped. */
    void SetIcon(const wxIcon& icon) { m_icon = icon; }

    /** @brief Clone. */
    wxEvent *Clone() const override { return new GraphTreeEvent(*this); }

private:
    GraphCtrl *m_target;    ///< Target of the drop.
    wxPoint m_pos;          ///< Position of the drop.
    wxTreeItemId m_item;    ///< Item being dropped.
    wxIcon m_icon;          ///< Icon of the item being dropped.

    DECLARE_DYNAMIC_CLASS(GraphTreeEvent)
};

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(Evt_GraphTree_Drop, wxEVT_USER_FIRST + 1121)
END_DECLARE_EVENT_TYPES()

/**
 * @brief Type of the handler for GraphTreeEvent events.
 *
 * All handlers for graph tree events must have this signature.
 */
typedef void (wxEvtHandler::*GraphTreeEventFunction)(GraphTreeEvent&);

} // namespace tt_solutions

// Avoid warnings about using the usual wx macros.
//
// NOLINTBEGIN(cppcoreguidelines-macro-usage)

/**
 * @brief Helper macro for use with Connect().
 *
 * When using wxEvtHandler::Connect() to connect to the graph events
 * dynamically, this macro should be applied to the event handler.
 */
#define GraphTreeEventHandler(func) \
    wxEVENT_HANDLER_CAST(tt_solutions::GraphTreeEventFunction, func)

/**
 * @brief Event that fires when an item from the GraphTreeCtrl is dropped
 * on a GraphCtrl.
 *
 * For example:
 * @code
 *  void MyFrame::OnGraphTreeDrop(GraphTreeEvent& event)
 *  {
 *      ProjectNode *node = new ProjectNode;
 *      node->SetText(event.GetString());
 *      node->SetResult(_T("this is a multi-\nline test"));
 *      node->SetColour(0x16a8fa);
 *      node->SetIcon(event.GetIcon());
 *      m_graph->Add(node, event.GetPosition());
 *  }
 * @endcode
 */
#define EVT_GRAPHTREE_DROP(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY(tt_solutions::Evt_GraphTree_Drop, id, \
                              wxID_ANY, GraphTreeEventHandler(fn), NULL),

// NOLINTEND(cppcoreguidelines-macro-usage)

#endif // GRAPHTREE_H
