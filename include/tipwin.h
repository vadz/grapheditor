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

/**
 * @brief Replacement of a tooltip window.
 *
 * This window is used instead of the standard tooltips to allow more control
 * over them.
 */
class TipWindow : public wxPopupWindow
{
public:
    /// Full ctor taking the same arguments as the base class one.
    TipWindow(wxWindow *parent,
              wxWindowID winid,
              const wxString& text,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxBORDER_NONE,
              const wxString& name = DefaultName);

    /**
     * @brief Set the tip text.
     *
     * Set the new tip text and resize the window to fit its new contents if @a
     * resize is true.
     */
    void SetText(const wxString& text, bool resize = true);

    /// Return the tip text.
    wxString GetText() const { return m_text; }

    /**
     * @brief Override the base class method to forward mouse move events.
     *
     * A tooltip window should be transparent for mouse move events and we
     * implement this by explicitly forwarding any such events that we get to
     * our parent window.
     */
    bool ProcessEvent(wxEvent& event);

    /**
     * @brief Override the base class method to refuse focus.
     *
     * A tooltip window doesn't accept input and hence doesn't need the focus.
     */
    bool AcceptsFocus() const { return false; }

    /**
     * @brief Paint event handler.
     *
     * Draw the tooltip text inside the window.
     */
    void OnPaint(wxPaintEvent& event);

    /// Default name for TipWindow objects, used in ctor.
    static const wxChar DefaultName[];

protected:
    /**
     * @brief Override the base class method to keep window on the screen.
     *
     * Ensure that the window is always fully visible.
     */
    void DoSetSize(int x, int y,
                   int width, int height,
                   int sizeFlags = wxSIZE_AUTO);

private:
    /// A possibly multiline text to show.
    wxString m_text;

    /// The margin around the text.
    wxSize m_margin;

    DECLARE_EVENT_TABLE()
};

} // namespace tt_solutions

#endif // TIPWIN_H
