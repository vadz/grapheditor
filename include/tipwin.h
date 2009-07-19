/////////////////////////////////////////////////////////////////////////////
// Name:        tipwin.h
// Purpose:     Tooltip window
// Author:      Mike Wetherell
// Modified by:
// Created:     July 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef TIPWIN_H
#define TIPWIN_H

#include <wx/wx.h>
#include <wx/popupwin.h>

namespace tt_solutions {

class TipWindow : public wxPopupWindow
{
public:
    TipWindow() { }
    TipWindow(wxWindow *parent,
              wxWindowID winid,
              const wxString& text,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxBORDER_NONE,
              const wxString& name = DefaultName);
    ~TipWindow() { }

    void SetText(const wxString& text, bool resize = true);
    wxString GetText() const { return m_text; }

    bool ProcessEvent(wxEvent& event);
    bool AcceptsFocus() const { return false; }

    void OnPaint(wxPaintEvent& event);

    static const wxChar DefaultName[];

protected:
    void DoSetSize(int x, int y,
                   int width, int height,
                   int sizeFlags = wxSIZE_AUTO);

private:
    wxString m_text;
    wxSize m_margin;

    DECLARE_EVENT_TABLE()
};

} // namespace tt_solutions

#endif // TIPWIN_H
