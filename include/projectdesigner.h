/////////////////////////////////////////////////////////////////////////////
// Name:        projectdesigner.h
// Purpose:     Classes for laying out project graphs
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PROJECTDESIGNER_H
#define PROJECTDESIGNER_H

/**
 * @file projectdesigner.h
 * @brief Header for ProjectDesigner classes.
 */

#include "graphctrl.h"

namespace datactics {

/**
 * @brief A custom GraphNode for the ProjectDesigner.
 *
 * @see ProjectDesigner
 */
class ProjectNode : public tt_solutions::GraphNode
{
public:
    /** @brief Constructor. */
    ProjectNode();
    /** @brief Destructor. */
    ~ProjectNode();

    /** @brief The node's id. */
    wxString GetId() const                  { return m_id; }
    /** @brief The node's operation label. */
    wxString GetOperation() const           { return GetText(); }
    /** @brief The node's result label. */
    wxString GetResult() const              { return m_result; }
    wxIcon GetIcon() const                  { return m_icon; }

    /** @brief The node's id. */
    void SetId(const wxString& text);
    /** @brief The node's operation label. */
    void SetOperation(const wxString& text) { SetText(text); }
    /** @brief The node's result label. */
    void SetResult(const wxString& text);
    void SetIcon(const wxIcon& icon);

    //bool Serialize(wxOutputStream& out);
    //bool Deserialize(wxInputStream& in);

    void OnDraw(wxDC& dc);

    int GetBorderThickness()                { return m_borderThickness; }
    int GetCornerRadius()                   { return m_cornerRadius; }

    void OnSize(int& x, int& y);
    void OnLayout(wxDC &dc);

private:
    wxString m_id;
    wxString m_result;
    wxIcon m_icon;
    wxString m_operation;
    int m_cornerRadius;
    int m_borderThickness;
    wxRect m_rcIcon;
    wxRect m_rcText;
    wxRect m_rcResult;
    wxSize m_minSize;
    int m_divide;

    DECLARE_DYNAMIC_CLASS(ProjectNode)
};

/**
 * @brief Graph layout control for Datactics projects.
 */
class ProjectDesigner : public tt_solutions::GraphCtrl
{
public:
    /** @brief Constructor. */
    ProjectDesigner();

    /** @brief Constructor. */
    ProjectDesigner(wxWindow *parent,
                    wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = wxBORDER | wxRETAINED,
                    const wxValidator& validator = wxDefaultValidator,
                    const wxString& name = DefaultName);

    /** @brief Destructor. */
    ~ProjectDesigner();

    void OnCanvasBackground(wxEraseEvent& event);
    void DrawCanvasBackground(wxDC& dc);

    void SetBackgroundGradient(const wxColour& from, const wxColour& to);

    static const wxChar DefaultName[];

private:
    void Init();
    wxColour m_background[2];

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(ProjectDesigner)
    DECLARE_NO_COPY_CLASS(ProjectDesigner)
};

} // namespace datactics

#endif // PROJECTDESIGNER_H
