/////////////////////////////////////////////////////////////////////////////
// Name:        tipwin.cpp
// Purpose:     Tooltip window
// Author:      Mike Wetherell
// Modified by:
// Created:     July 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "tipwin.h"

#undef max
#undef min

using std::min;
using std::max;

/**
 * @file
 * @brief Custom tooltip window class implementation.
 */

namespace tt_solutions {

BEGIN_EVENT_TABLE(TipWindow, wxPopupWindow)
    EVT_PAINT(TipWindow::OnPaint)
END_EVENT_TABLE()

const wxChar TipWindow::DefaultName[] = _T("tip_window");

TipWindow::TipWindow(wxWindow *parent,
                     wxWindowID winid,
                     const wxString& text,
                     const wxPoint& pos,
                     const wxSize& size,
                     long style,
                     const wxString& name)
  : wxPopupWindow(parent, style),
    m_text(text),
    m_margin(3, 3)
{
    SetId(winid);
    SetName(name);

    SetExtraStyle(wxWS_EX_TRANSIENT | GetExtraStyle());

    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));
    SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

    wxRect rc(pos, size);

    if (pos == wxDefaultPosition) {
        rc.SetPosition(wxGetMousePosition());
        rc.y += wxSystemSettings::GetMetric(wxSYS_CURSOR_Y) * 2 / 3;
    }

    SetSize(rc);
}

void TipWindow::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    wxClientDC dc(this);
    dc.SetFont(GetFont());

    wxSize autsize = dc.GetMultiLineTextExtent(m_text);
    autsize += m_margin * 2;
    wxSize current = GetSize();

    if (width == -1)
        width = (sizeFlags & wxSIZE_AUTO_WIDTH) != 0 ? autsize.x : current.x;
    if (height== -1)
        height = (sizeFlags & wxSIZE_AUTO_HEIGHT) != 0 ? autsize.y : current.y;

    sizeFlags &= ~wxSIZE_AUTO;

    wxPoint pt = GetPosition();
    if (x == -1 && !(sizeFlags & wxSIZE_ALLOW_MINUS_ONE))
        x = pt.x;
    if (y == -1 && !(sizeFlags & wxSIZE_ALLOW_MINUS_ONE))
        y = pt.y;

    wxRect rcDesktop = wxGetClientDisplayRect();
    x = max(min(x, rcDesktop.GetRight() + 1 - width), 0);
    y = max(min(y, rcDesktop.GetBottom() + 1 - height), 0);

    wxPopupWindow::DoSetSize(x, y, width, height, sizeFlags);
}

void TipWindow::SetText(const wxString& text, bool resize)
{
    m_text = text;
    if (resize)
        SetSize(-1, -1, -1, -1, wxSIZE_AUTO);
}

bool TipWindow::ProcessEvent(wxEvent& event)
{
    wxEventType type = event.GetEventType();

    if (type == wxEVT_MOTION) {
        const wxMouseEvent& me = dynamic_cast<wxMouseEvent&>(event);

        wxMouseEvent me2(me);
        wxWindow *parent = GetParent();

        wxPoint pt = ClientToScreen(me.GetPosition());
        pt = parent->ScreenToClient(pt);
        me2.SetEventObject(parent);
        me2.m_x = pt.x;
        me2.m_y = pt.y;

        return parent->GetEventHandler()->ProcessEvent(me2);
    }

    return wxPopupWindow::ProcessEvent(event);
}

void TipWindow::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    wxRect rc = GetClientRect();

    dc.SetBrush(GetBackgroundColour());
    dc.SetPen(GetForegroundColour());
    dc.DrawRectangle(rc);

    rc.Deflate(m_margin);

    dc.SetFont(GetFont());
    dc.SetTextForeground(GetForegroundColour());
    dc.DrawLabel(m_text, rc);
}

} // namespace tt_solutions
