/////////////////////////////////////////////////////////////////////////////
// Name:        graphtree.cpp
// Purpose:     Tree control drag source for graph nodes
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "graphtree.h"

namespace tt_solutions {

DEFINE_EVENT_TYPE(Evt_GraphTree_Drop)

IMPLEMENT_DYNAMIC_CLASS(GraphTreeCtrl, wxTreeCtrl)
IMPLEMENT_DYNAMIC_CLASS(GraphTreeEvent, wxCommandEvent)

BEGIN_EVENT_TABLE(GraphTreeCtrl, wxTreeCtrl)
    EVT_TREE_BEGIN_DRAG(wxID_ANY, GraphTreeCtrl::OnBeginDrag)
    EVT_MOTION(GraphTreeCtrl::OnMouseMove)
    EVT_LEFT_UP(GraphTreeCtrl::OnLeftButtonUp)
END_EVENT_TABLE()

const wxChar GraphTreeCtrl::DefaultName[] = _T("graphtreectrl");

void GraphTreeCtrl::OnBeginDrag(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();

    if (GetChildrenCount(item, false) == 0) {
        m_dragItem = item;
        int image = GetItemImage(item);

        if (image != -1)
            m_dragImg = new wxDragImage(GetImageList()->GetIcon(image));
        else
            m_dragImg = new wxDragImage(*this, item);

        wxRect rc = m_dragImg->GetImageRect(wxPoint(0, 0));
        wxPoint pt(3 * rc.GetWidth() / 4, 3 * rc.GetHeight() / 4);
        m_dragImg->BeginDrag(pt, this, true);
        m_dragImg->Show();
    }
}

void GraphTreeCtrl::OnMouseMove(wxMouseEvent& event)
{
    if (m_dragImg)
        m_dragImg->Move(event.GetPosition());
    else
        event.Skip();
}

void GraphTreeCtrl::OnLeftButtonUp(wxMouseEvent& event)
{
    if (m_dragImg) {
        wxTreeItemId item = m_dragItem;

        m_dragImg->Hide();
        m_dragImg->EndDrag();
        delete m_dragImg;
        m_dragItem = 0;
        m_dragImg = NULL;

        wxPoint pt = ClientToScreen(event.GetPosition());
        wxWindow *win;

        for (win = wxFindWindowAtPoint(pt); win; win = win->GetParent())
        {
            GraphCtrl *graphctrl = wxDynamicCast(win, GraphCtrl);

            if (graphctrl) {
                pt = graphctrl->ScreenToGraph(pt);
                GraphTreeEvent event(Evt_GraphTree_Drop, GetId());
                event.SetEventObject(this);
                int image = GetItemImage(item);
                if (image != -1)
                    event.SetIcon(GetImageList()->GetIcon(image));
                event.SetString(GetItemText(item));
                event.SetTarget(graphctrl);
                event.SetPosition(pt);
                event.SetItem(item);
                GetEventHandler()->ProcessEvent(event);
                break;
            }
        }
    }
    else {
        event.Skip();
    }
}

} // namespace tt_solutions
