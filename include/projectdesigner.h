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
    /**
     * @brief An enumeration for indicating what part of a node is at a given
     * point, for example the text label or image.
     */
    enum HitValue {
        Hit_No,
        Hit_Yes,
        Hit_Operation,
        Hit_Result,
        Hit_Image
    };

    /** @brief Constructor. */
    ProjectNode();
    /** @brief Destructor. */
    ~ProjectNode();

    void SetText(const wxString& text);
    void SetFont(const wxFont& font);
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

    /**
     * @brief Indicates what part of the node is at the given point, for
     * example the text label or image. Returns a value from the HitValue
     * enumeration.
     */
    int HitTest(const wxPoint& pt) const;

    int GetBorderThickness() const          { return m_borderThickness; }
    void SetBorderThickness(int thickness);

    int GetCornerRadius() const             { return m_cornerRadius; }
    void SetCornerRadius(int radius);

    void OnDraw(wxDC& dc);
    void OnLayout(wxDC &dc);

    wxPoint GetPerimeterPoint(const wxPoint& inside,
                              const wxPoint& outside) const;

private:
    wxPoint GetCornerPoint(const wxPoint& centre,
                           int radius, int sign,
                           const wxPoint& inside,
                           const wxPoint& outside) const;

    wxString m_id;
    wxString m_result;
    wxIcon m_icon;
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

    void SetShowGrid(bool show);
    bool IsGridShown() const { return m_showGrid; }

    static const wxChar DefaultName[];

private:
    void Init();
    wxColour m_background[2];
    bool m_showGrid;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(ProjectDesigner)
    DECLARE_NO_COPY_CLASS(ProjectDesigner)
};

} // namespace datactics

#endif // PROJECTDESIGNER_H
