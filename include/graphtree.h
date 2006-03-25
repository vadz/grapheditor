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
 * @brief Tree control
 */
class GraphTreeCtrl : public wxTreeCtrl
{
public:
    GraphTreeCtrl() { Init(); }

    GraphTreeCtrl(wxWindow *parent, wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT,
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

    GraphCtrl *GetTarget() const { return m_target; }
    void SetTarget(GraphCtrl *target) { m_target = target; }

    wxPoint GetPosition() const { return m_pos; }
    void SetPosition(const wxPoint& pt) { m_pos = pt; }

    int GetItem() const { return m_item; }
    void SetItem(int item) { m_item = item; }

    wxIcon GetIcon() const { return m_icon; }
    void SetIcon(const wxIcon& icon) { m_icon = icon; }
 
private:
    GraphCtrl *m_target;
    wxPoint m_pos;
    int m_item;
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

#define EVT_GRAPHTREE_DROP(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY(tt_solutions::Evt_GraphTree_Drop, id, \
                              wxID_ANY, GraphTreeEventHandler(fn), NULL),

#endif // GRAPHTREE_H
