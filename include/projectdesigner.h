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
    ProjectNode(const wxString& operation = wxEmptyString,
                const wxString& result = wxEmptyString,
                const wxString& id = wxEmptyString,
                const wxIcon& icon = wxIcon(),
                const wxColour& colour = *wxLIGHT_GREY,
                const wxColour& bgcolour = *wxWHITE,
                const wxColour& textcolour = *wxBLACK);
    /** @brief Destructor. */
    ~ProjectNode();

    void SetText(const wxString& text);
    void SetFont(const wxFont& font);
    /** @brief The node's id. */
    wxString GetId() const                  { return m_id; }
    /** @brief The node's operation label. Synonym for GetText(). */
    wxString GetOperation() const           { return GetText(); }
    /** @brief The node's result label. */
    wxString GetResult() const              { return m_result; }
    wxIcon GetIcon() const                  { return m_icon; }

    /** @brief The node's id. */
    void SetId(const wxString& text);
    /** @brief The node's operation label. Synonym for SetText(). */
    void SetOperation(const wxString& text) { SetText(text); }
    /** @brief The node's result label. */
    void SetResult(const wxString& text);
    void SetIcon(const wxIcon& icon);

    bool Serialise(tt_solutions::Archive::Item& arc);

    /**
     * @brief Indicates what part of the node is at the given point, for
     * example the text label or image.
     *
     * @returns Returns a value from the HitValue enumeration.
     */
    int HitTest(const wxPoint& pt) const;

    int GetBorderThickness() const;
    template <class T> int GetBorderThickness() const;

    void SetBorderThickness(int thickness);
    template <class T> void SetBorderThickness(int thickness);

    int GetCornerRadius() const;
    template <class T> int GetCornerRadius() const;

    void SetCornerRadius(int radius);
    template <class T> void SetCornerRadius(int radius);

    //@{
    /**
     * @brief The maximum size that will be automatically set.
     *
     * The minimum size of the node if the content text would otherwise
     * force a larger size.
     * */
    wxSize GetMaxAutoSize() const { return m_maxAutoSize; }
    template <class T> wxSize GetMaxAutoSize() const;
    void SetMaxAutoSize(const wxSize& size) { m_maxAutoSize = size; }
    template <class T> void SetMaxAutoSize(const wxSize& size);
    //@}

    void OnDraw(wxDC& dc);
    void OnLayout(wxDC &dc);

    wxPoint GetPerimeterPoint(const wxPoint& inside,
                              const wxPoint& outside) const;

protected:
    int GetSpacing() const;

private:
    typedef tt_solutions::Pixels Pixels;
    typedef tt_solutions::Twips Twips;

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
    wxSize m_maxAutoSize;
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

    /** @brief Sets the background colour of the control. */
    bool SetBackgroundColour(const wxColour& colour);
    /** @brief Sets the background colour gradient of the control. */
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

// ----------------------------------------------------------------------------
// Inline definitions
// ----------------------------------------------------------------------------

inline int ProjectNode::GetBorderThickness() const
{
    return GetBorderThickness<Pixels>();
}

template <class T> int ProjectNode::GetBorderThickness() const
{
    return Twips::To<T>(m_borderThickness, GetDPI().y);
}

inline void ProjectNode::SetBorderThickness(int thickness)
{
    SetBorderThickness<Pixels>(thickness);
}

template <class T> void ProjectNode::SetBorderThickness(int thickness)
{
    m_borderThickness = Twips::From<T>(thickness, GetDPI().y);
    Layout();
    Refresh();
}

inline int ProjectNode::GetCornerRadius() const
{
    return GetCornerRadius<Pixels>();
}

template <class T> int ProjectNode::GetCornerRadius() const
{
    return Twips::To<T>(m_cornerRadius, GetDPI().y);
}

inline void ProjectNode::SetCornerRadius(int radius)
{
    SetCornerRadius<Pixels>(radius);
}

template <class T> void ProjectNode::SetCornerRadius(int radius)
{
    m_cornerRadius = Twips::From<T>(radius, GetDPI().y);
    Layout();
    Refresh();
}

template <class T> wxSize ProjectNode::GetMaxAutoSize() const
{
    return Pixels::To<T>(GetMaxAutoSize(), GetDPI());
}

template <class T> void ProjectNode::SetMaxAutoSize(const wxSize& size)
{
    SetMaxAutoSize(Pixels::From<T>(size, GetDPI()));
}

} // namespace datactics

#endif // PROJECTDESIGNER_H
