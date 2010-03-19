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

/**
 * @brief Datactics
 */
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

    //@{
    /** @brief The node's id. */
    wxString GetId() const                  { return m_id; }
    void SetId(const wxString& text);
    //@}

    /** @brief The node's operation label. Synonym for @c GetText(). */
    wxString GetOperation() const           { return GetText(); }
    /** @brief The node's operation label. Synonym for @c SetText(). */
    void SetOperation(const wxString& text) { SetText(text); }

    //@{
    /** @brief The node's result label. */
    wxString GetResult() const              { return m_result; }
    void SetResult(const wxString& text);
    //@}

    //@{
    /** @brief The node's icon. */
    wxIcon GetIcon() const                  { return m_icon; }
    void SetIcon(const wxIcon& icon);
    //@}

    /**
     * @brief Save or load this node's attributes.
     *
     * Can be overridden in a derived class to handle any additional
     * attributes.
     */
    bool Serialise(tt_solutions::Archive::Item& arc);

    /**
     * @brief Indicates what part of the node is at the given point, for
     * example the text label or image.
     *
     * @returns Returns a value from the HitValue enumeration.
     */
    int HitTest(const wxPoint& pt) const;

    //@{
    /**
     * @brief The node's border thickness.
     *
     * @tparam T @c Points or @c Twips specifying the units of the thickness.
     * If omitted defaults to pixels.
     */
    template <class T> int GetBorderThickness() const;
    template <class T> void SetBorderThickness(int thickness);
    //@}
    /** @cond */
    int GetBorderThickness() const;
    void SetBorderThickness(int thickness);
    /** @endcond */

    //@{
    /**
     * @brief The node's corner radius.
     *
     * @tparam T @c Points or @c Twips specifying the units of the radius.
     * If omitted defaults to pixels.
     */
    template <class T> int GetCornerRadius() const;
    template <class T> void SetCornerRadius(int radius);
    //@}
    /** @cond */
    int GetCornerRadius() const;
    void SetCornerRadius(int radius);
    /** @endcond */

    //@{
    /**
     * @brief The maximum size that will be automatically set.
     *
     * The minimum size of the node if the content text would otherwise
     * force a larger size.
     *
     * @tparam T @c Points or @c Twips specifying the units of the size.
     * If omitted defaults to pixels.
     * */
    template <class T> wxSize GetMaxAutoSize() const;
    template <class T> void SetMaxAutoSize(const wxSize& size);
    //@}
    /** @cond */
    wxSize GetMaxAutoSize() const { return m_maxAutoSize; }
    void SetMaxAutoSize(const wxSize& size) { m_maxAutoSize = size; }
    /** @endcond */

    void OnDraw(wxDC& dc);
    void OnLayout(wxDC &dc);

    wxPoint GetPerimeterPoint(const wxPoint& inside,
                              const wxPoint& outside) const;

protected:
    /**
     * Return the spacing to use for layout.
     *
     * The spacing is determined by the border thickness and corner radius.
     */
    int GetSpacing() const;

private:
    /// Bring the Pixels coordinates type into this class scope.
    typedef tt_solutions::Pixels Pixels;
    /// Bring the Twips coordinates type into this class scope.
    typedef tt_solutions::Twips Twips;

    /// Helper of GetPerimeterPoint().
    wxPoint GetCornerPoint(const wxPoint& centre,
                           int radius, int sign,
                           const wxPoint& inside,
                           const wxPoint& outside) const;

    wxString m_id;              ///< Unique project id.
    wxString m_result;          ///< Result label.
    wxIcon m_icon;              ///< Node icon.
    int m_cornerRadius;         ///< Corner radius in pixels.
    int m_borderThickness;      ///< Border thickness.
    wxRect m_rcIcon;            ///< Icon area.
    wxRect m_rcText;            ///< Text area.
    wxRect m_rcResult;          ///< Result area.
    wxSize m_maxAutoSize;       ///< Max auto layout size. 144*72pp by default.
    int m_divide;               ///< Position of the dividing line.

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

    /**
     * Event handler for background erase event.
     *
     * To reduce flicker, the background is drawn here instead of drawing the
     * default background and then overwriting it when painting the window
     * contents.
     */
    void OnCanvasBackground(wxEraseEvent& event);

    /**
     * Background drawing function.
     *
     * This function is used by OnCanvasBackground() to really draw the
     * background.
     */
    void DrawCanvasBackground(wxDC& dc);

    /** @brief Sets the background colour of the control. */
    bool SetBackgroundColour(const wxColour& colour);
    /** @brief Sets the background colour gradient of the control. */
    void SetBackgroundGradient(const wxColour& from, const wxColour& to);

    //@{
    /**
     * @brief The 'show-grid' flag.
     *
     * When @c true, every @e n-th grid line is drawn on the background, where
     * @e n is determined by @c SetGridFactor().
     */
    void SetShowGrid(bool show);
    bool IsGridShown() const { return m_showGrid; }
    //@}

    //@{
    /**
     * @brief The grid factor determines how many of the snap grid's lines are
     * drawn on the background.
     *
     * The default is 5, that is every fifth grid line is shown. This allows
     * a fairly fine grid to be used, giving users and the automatic layout
     * reasonable freedom to place nodes, while not overcrowding the
     * background with grid lines.
     */
    int GetGridFactor() const { return m_gridFactor; }
    void SetGridFactor(int factor) { m_gridFactor = factor; Refresh(); }
    //@}

    static const wxChar DefaultName[];

protected:
    /** @brief Adjust the grid factor when zoomed-in to avoid overcrowding. */
    virtual int AdjustedGridFactor() const;

private:
    /// Common part of all ctors.
    void Init();

    /**
     * @brief Colours defining the background.
     *
     * If the colours are equal, a solid background are used. Otherwise the
     * background is drawn using a gradient from the first to the second array
     * elements.
     *
     * Notice that the array always contains two elements.
     */
    wxColour m_background[2];

    /// True if the grid is shown. True by default.
    bool m_showGrid;

    /// Grid factor. Default is 5. @see GetGridFactor().
    int m_gridFactor;

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
