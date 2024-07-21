/////////////////////////////////////////////////////////////////////////////
// Name:        graphprint.h
// Purpose:     Printout for the graph control
// Author:      Mike Wetherell
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef GRAPHPRINT_H
#define GRAPHPRINT_H

#include <wx/prntbase.h>

#include "graphctrl.h"

/**
 * @file graphprint.h
 * @brief Printing for the graph control.
 */

namespace tt_solutions {

/**
 * @brief The max page limit for GraphPrintout.
 *
 * A simple struct that can be passed to the @link
 * GraphPrintout::GraphPrintout() <code>GraphPrintout</code>
 * constructor@endlink to set a limit on the number of pages produced.
 *
 * Maximum limits can be set for the rows, columns and pages in total.
 * For no-limit these should be set to <code>MaxPages::Unlimited</code>.
 *
 * @see GraphPrintout
 */
struct MaxPages
{
    enum { Unlimited = 0 };

    /**
     * @brief Initialize an object without any limits.
     *
     * This ctor creates an object which doesn't impose any limit on the number
     * of pages.
     */
    MaxPages(int pages = Unlimited)
      : rows(Unlimited), cols(Unlimited), pages(pages)
    { }

    /**
     * @brief Initialize an object for the given maximal number of pages.
     *
     * @param rows Maximal number of pages to print in vertical direction.
     * @param cols Maximal number of pages to print in horizontal direction.
     * @param pages Maximal total number of pages; unlimited by default.
     */
    MaxPages(int rows, int cols, int pages = Unlimited)
      : rows(rows), cols(cols), pages(pages)
    { }

    int rows;     ///< Max number of pages in vertical direction or Unlimited.
    int cols;     ///< Max number of pages in horizontal direction or Unlimited.
    int pages;    ///< Max total number of pages or Unlimited.
};

/**
 * @brief A header or footer for @c GraphPrintout.
 *
 * @see
 *  GraphPrintout @n
 *  Header() @n
 *  Footer()
 */
class PrintLabel
{
public:
    /**
     * @brief Constructor.
     *
     * It is usually easier to use the helper functions <code>Header()</code>
     * and <code>Footer()</code> rather than create a <code>PrintLabel</code>
     * instance directly.
     *
     * @param text The text to print. Can include variables, see @c SetText().
     * @param flags See <code>SetFlags()</code> for details.
     * @param height Height of the header or footer in millimeters.
     * @param font The font to use.
     */
    PrintLabel(const wxString& text, int flags, int height, wxFont font)
      : m_text(text), m_flags(flags), m_height(height), m_font(font)
    { }

    //@{
    /**
     * @brief The header or footer text.
     *
     * Can include the following, which are substituted at runtime:
     * @li @c \%ROW\%   Current page row number.
     * @li @c \%ROWS\%  Total page rows that will be printed.
     * @li @c \%COL\%   Current page column number.
     * @li @c \%COLS\%  Total page columns that will be printed.
     * @li @c \%PAGE\%  Current page number.
     * @li @c \%PAGES\% Total pages that will be printed.
     */
    void SetText(const wxString& text) { m_text = text; }
    wxString GetText() const { return m_text; }
    //@}

    //@{
    /** @brief The font for the header or footer. */
    void SetFont(const wxFont& font) { m_font = font; }
    wxFont GetFont() const { return m_font; }
    //@}

    //@{
    /**
     * @brief Flags for position and alignment.
     *
     * Combination of the following:
     * @li @c wxTOP
     * @li @c wxBOTTOM
     * @li @c wxALIGN_LEFT
     * @li @c wxALIGN_RIGHT
     * @li @c wxALIGN_CENTRE
     * @li @c wxALIGN_TOP
     * @li @c wxALIGN_BOTTOM
     *
     * Headers require @c wxTOP, footers @c wxBOTTOM. Headers should also
     * usually have @c wxALIGN_TOP and footers @c wxALIGN_BOTTOM.
     */
    void SetFlags(int flags) { m_flags = flags; }
    int GetFlags() const { return m_flags; }
    //@}

    /** @brief Text alignment, bitwise ored wxAlignment values. */
    int GetAlignment() const { return m_flags & wxALIGN_MASK; }
    /** @brief wxTOP for headers or wxBOTTOM for footers. */
    int GetPosition() const { return m_flags & wxALL; }
    /** @brief The height of the header or footer in millimetres. */
    int GetHeight() const { return m_height; }

    /// List of print labels.
    typedef std::list<PrintLabel> list;

    /**
     * @brief Allow conversion to a list of @c PrintLabels.
     *
     * The @c + operator can be used to create lists of @c PrintLabel objects
     * that can be passed to the @link GraphPrintout::GraphPrintout()
     * <code>GraphPrintout</code> constructor@endlink, to allow multiple
     * headers and footers on a printout.
     */
    operator list() const { return list(1, *this); }

private:
    wxString m_text;        ///< Text of the header or footer.
    int m_flags;            ///< Label position flags. @see SetFlags.
    int m_height;           ///< Height of the label in millimeters.
    wxFont m_font;          ///< Font used to render the label.
};

/// List of print labels.
typedef PrintLabel::list PrintLabels;

//@{
/**
 * @brief Allow construction of a list of @c PrintLabels with the @c +
 * operator.
 *
 * The list can be passed to the @link GraphPrintout::GraphPrintout()
 * <code>GraphPrintout</code> constructor@endlink, to allow allow multiple
 * headers and footers to be used on a printout.
 */
inline PrintLabels& operator+=(PrintLabels& l1, const PrintLabels& l2)
{
    l1.insert(l1.end(), l2.begin(), l2.end());
    return l1;
}
inline PrintLabels operator+(const PrintLabels& l1, const PrintLabels& l2)
{
    PrintLabels l(l1);
    return l += l2;
}
//@}

/**
 * @brief Create a header for the GraphPrintout.
 *
 * Creates a @c PrintLabel with defaults suitable for a footer. See
 * @c PrintLabel::PrintLabel() for details of the parameters.
 */
inline PrintLabels Footer(
    const wxString& text = _("Page %PAGE% of %PAGES%"),
    int align = wxALIGN_CENTRE,
    int height = 10,
    const wxFont& font = wxFontInfo(12).Family(wxFONTFAMILY_SWISS))
{
    return PrintLabel(text, align | wxALIGN_BOTTOM | wxBOTTOM, height, font);
}

/**
 * @brief Create a footer for the GraphPrintout.
 *
 * Creates a @c PrintLabel with defaults suitable for a header. See
 * @c PrintLabel::PrintLabel() for details of the parameters.
 */
inline PrintLabels Header(
    const wxString& text = _("Page %PAGE% of %PAGES%"),
    int align = wxALIGN_CENTRE,
    int height = 10,
    const wxFont& font = wxFontInfo(12).Family(wxFONTFAMILY_SWISS))
{
    return PrintLabel(text, align | wxALIGN_TOP | wxTOP, height, font);
}

/**
 * @brief The implementation of the <code>GraphPrintout</code> class.
 *
 * The implementation of the @c GarphPrintout class has been seperated out
 * into this class to allow the graph printing code to be incorporated into
 * other @c wxPrintout classes. For example it could be used when one or more
 * graphs are to be printed as part of a larger document.
 *
 * If you just want to print one graph then @c GraphPrintout should directly
 * be used instead.
 *
 * @see GraphPrintout
 */
class GraphPages
{
public:
    /**
     * @brief Constructor.
     *
     * The parameters are the same as those of the @link
     * GraphPrintout::GraphPrintout() <code>GraphPrintout</code>
     * constructor@endlink.
     */
    GraphPages(Graph *graph,
               const wxPageSetupDialogData& setup,
               double scale = 100,
               MaxPages shrinktofit = MaxPages::Unlimited,
               PrintLabels labels = Footer(),
               double posX = .5,
               double posY = .3);

    /** @brief Destructor. */
    virtual ~GraphPages() { }

    /** @brief Implements wxPrintout::OnPreparePrinting(). */
    virtual void PreparePrinting();
    /** @brief Implements wxPrintout::OnPrintPage(). */
    virtual bool PrintPage(int printoutPage, int graphPage);

    /** @brief The total number of pages needed for this graph. */
    int GetPages() const { return m_pages.x * m_pages.y; }
    /** @brief The number of pages down needed for this graph. */
    int GetRows() const { return m_pages.y; }
    /** @brief The number of pages across needed for this graph. */
    int GetCols() const { return m_pages.x; }

    /**
     * @brief The page rectangle less borders, unprintable area, headers and
     * footers.
     */
    wxRect GetPrintRect() const { return m_print; }

    //@{
    /** @brief The owning wxPrintout object. */
    wxPrintout *GetPrintout() const { return m_printout; }
    void SetPrintout(wxPrintout *printout) { m_printout = printout; }
    //@}

protected:
    /** @brief Render a header or footer. */
    virtual void DrawLabel(wxDC *dc, const PrintLabel& label,
                           const wxRect& rc, int page, int row, int col);

private:
    wxPrintout *m_printout;             ///< The associated printout.
    Graph *m_graph;                     ///< The graph being printed.
    double m_scale;                     ///< Zoom scale.
    MaxPages m_max;                     ///< Limit on the number of pages.
    wxPageSetupDialogData m_setup;      ///< The associated page data.
    wxSize m_pages;                     ///< Number of pages needed.
    wxPoint m_firstPage;                ///< Origin of the first page.
    wxRect m_print;                     ///< Page area (excluding labels).
    wxRect m_header;                    ///< Header area.
    wxRect m_footer;                    ///< Footer area.
    PrintLabels m_labels;               ///< Headers and footers.
    double m_posX;                      ///< X page position given to ctor.
    double m_posY;                      ///< Y pge position given to ctor.
};

/**
 * @brief A wxPrintout class for Graph objects.
 *
 * @see
 *  Graph @n
 *  MaxPages @n
 *  PrintLabel
 */
class GraphPrintout : public wxPrintout
{
public:
    /**
     * @brief Constructor.
     *
     * Both a max scaling percentage and a maximum number of pages can be
     * specified. The smaller of these two limits will apply.
     *
     * Multiple headers and footers can be used, as long as each goes to
     * a different corner. Multiple headers/footers can be combined using
     * the '+' operator:
     *
     * @code
     *  Footer(m_filename, wxALIGN_LEFT) +
     *  Footer(_T("%PAGE% / %PAGES%"), wxALIGN_RIGHT))
     * @endcode
     *
     * @param graph The graph to print.
     * @param setup The setup data from the print setup dialog.
     * @param scale The maximum scaling percentage that should be used.
     * @param shrinktofit The maximum number of pages that should be used.
     * @param labels One or more headers and footers.
     * @param posX The horizontal position of the graph in the printout,
     *  a value between 0.0 (left) and 1.0 (right).
     * @param posY The vertical position of the graph in the printout,
     *  a value between 0.0 (top) and 1.0 (bottom).
     * @param title Text displayed in the printing dialog.
     */
    GraphPrintout(Graph *graph,
                  const wxPageSetupDialogData& setup,
                  double scale = 100,
                  MaxPages shrinktofit = MaxPages::Unlimited,
                  PrintLabels labels = Footer(),
                  double posX = .5,
                  double posY = .3,
                  const wxString& title = _("Graph"));
    /**
     * @brief Constructor.
     *
     * A constructor that allows a derived @c GraphPages type to be used
     * instead of the default.
     */
    GraphPrintout(GraphPages *graphpages,
                  const wxString& title = _("Graph"));
    /** @brief Destructor. */
    ~GraphPrintout();

    /**
     * Override wxPrintout methods which must be implemented for printing.
     *
     * These methods are implemented by simply forwarding them to the
     * corresponding methods of GraphPages.
     */
    //@{
    void OnPreparePrinting() override;
    bool HasPage(int page) override;
    bool OnPrintPage(int page) override;
    void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo) override;
    //@}

private:
    /// GraphPages pointer created and owned by the printout.
    GraphPages *m_graphpages;
};

} // namespace tt_solutions

#endif // GRAPHPRINT_H
