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
#include <wx/imaglist.h>

/**
 * @file
 * @brief Implementation of the custom tree control class.
 */

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
        SelectItem(item);
        int image = GetItemImage(item);
        wxSize size;

        if (image != -1) {
            wxIcon icon = GetImageList()->GetIcon(image);
            size = wxSize(icon.GetWidth(), icon.GetHeight());
            m_dragImg = new wxDragImage(icon);
        }
        else {
            m_dragImg = new wxDragImage(*this, item);
        }

        wxPoint pt(3 * size.x / 4, 3 * size.y / 4);
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
        m_dragImg = NULL;

        wxPoint pt = ClientToScreen(event.GetPosition());
        wxWindow *win;

        for (win = wxFindWindowAtPoint(pt); win; win = win->GetParent())
        {
            GraphCtrl *graphctrl = wxDynamicCast(win, GraphCtrl);

            if (graphctrl) {
                pt = graphctrl->ScreenToGraph(pt);
                GraphTreeEvent eventGraph(Evt_GraphTree_Drop, GetId());
                eventGraph.SetEventObject(this);
                int image = GetItemImage(item);
                if (image != -1)
                    eventGraph.SetIcon(GetImageList()->GetIcon(image));
                eventGraph.SetString(GetItemText(item));
                eventGraph.SetTarget(graphctrl);
                eventGraph.SetPosition(pt);
                eventGraph.SetItem(item);
                GetEventHandler()->ProcessEvent(eventGraph);
                break;
            }
        }
    }
    else {
        event.Skip();
    }
}

} // namespace tt_solutions
