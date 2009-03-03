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
 * Dropping a node fires the event <code>EVT_GRAPHTREE_DROP</code>.
 *
 * @see GraphTreeEvent
 */
class GraphTreeCtrl : public wxTreeCtrl
{
public:
    GraphTreeCtrl() { Init(); }

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

    bool SendAutoScrollEvents(wxScrollWinEvent&) const { return false; }

    void OnBeginDrag(wxTreeEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftButtonUp(wxMouseEvent& event);

    static const wxChar DefaultName[];

private:
    void Init() { m_dragImg = NULL; }

    wxDragImage *m_dragImg;
    wxTreeItemId m_dragItem;

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
    GraphTreeEvent(wxEventType commandType = wxEVT_NULL, int winid = 0)
        : wxCommandEvent(commandType, winid)
    { }

    GraphTreeEvent(const GraphTreeEvent& event)
        : wxCommandEvent(event),
          m_target(event.GetTarget()),
          m_pos(event.GetPosition()),
          m_item(event.GetItem()),
          m_icon(event.GetIcon())
    { }

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

    wxEvent *Clone() const { return new GraphTreeEvent(*this); }

private:
    GraphCtrl *m_target;
    wxPoint m_pos;
    wxTreeItemId m_item;
    wxIcon m_icon;

    DECLARE_DYNAMIC_CLASS(GraphTreeEvent)
};

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(Evt_GraphTree_Drop, wxEVT_USER_FIRST + 1121)
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*GraphTreeEventFunction)(GraphTreeEvent&);

} // namespace tt_solutions

#define GraphTreeEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction) \
        wxStaticCastEvent(tt_solutions::GraphTreeEventFunction, &func)

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

#endif // GRAPHTREE_H
