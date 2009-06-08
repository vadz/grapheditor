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
 * Maximum limits can be set for the rows, columns and pages in total.
 * For no-limit these should be set to <code>MaxPages::Unlimited</code>.
 *
 * @see GraphPrintout
 */
struct MaxPages
{
    enum { Unlimited = 0 };

    MaxPages(int pages = Unlimited)
      : rows(Unlimited), cols(Unlimited), pages(pages)
    { }

    MaxPages(int rows, int cols, int pages = Unlimited)
      : rows(rows), cols(cols), pages(pages)
    { }

    int rows;
    int cols;
    int pages;
};

/**
 * @brief A header or footer for the printout.
 *
 * @see GraphPrintout
 * @see Header
 * @see Footer
 */
class PrintLabel
{
public:
    /**
     * @brief Constructor.
     *
     * It is usually easier to use the helper functions <code>Header</code>
     * and <code>Footer</code> rather than create a <code>PrintLabel</code>
     * instance directly.
     *
     * @param text The text to print. Can include the variable, see
     *             <code>SetText</code>.
     * @param flags See <code>SetFlags</code> for details.
     * @param height Height of the header or footer in millimeters.
     * @param font The font to use.
     */
    PrintLabel(const wxString& text, int flags, int height, wxFont font)
      : m_text(text), m_flags(flags), m_height(height), m_font(font)
    { }

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

    void SetFont(const wxFont& font) { m_font = font; }
    wxFont GetFont() const { return m_font; }

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

    int GetAlignment() const { return m_flags & wxALIGN_MASK; }
    int GetPosition() const { return m_flags & wxALL; }
    int GetHeight() const { return m_height; }

    typedef std::list<PrintLabel> list;
    operator list() const { return list(1, *this); }

private:
    wxString m_text;
    int m_flags;
    int m_height;
    wxFont m_font;
};

typedef PrintLabel::list PrintLabels;

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

/**
 * @brief Create a header for the GraphPrintout.
 *
 * @see PrintLabel
 * @see GraphPrintout
 */
inline PrintLabels Footer(
    const wxString& text = _("Page %PAGE% of %PAGES%"),
    int align = wxALIGN_CENTRE,
    int height = 10,
    const wxFont& font = wxFont(12, wxSWISS, wxNORMAL, wxNORMAL))
{
    return PrintLabel(text, align | wxALIGN_BOTTOM | wxBOTTOM, height, font);
}

/**
 * @brief Create a footer for the GraphPrintout.
 *
 * @see PrintLabel
 * @see GraphPrintout
 */
inline PrintLabels Header(
    const wxString& text = _("Page %PAGE% of %PAGES%"),
    int align = wxALIGN_CENTRE,
    int height = 10,
    const wxFont& font = wxFont(12, wxSWISS, wxNORMAL, wxNORMAL))
{
    return PrintLabel(text, align | wxALIGN_TOP | wxTOP, height, font);
}

/**
 * @brief The implementation of the <code>GraphPrintout</code> class.
 *
 * The implementation of the @c GarphPrintout class is here, it allows
 * you to incorporate one or more graphs into your own printout class.
 *
 * If you just want to print one graph then @c GraphPrintout should be used
 * instead.
 *
 * @see GraphPrintout
 */
class GraphPages
{
public:
    GraphPages(Graph *graph,
               const wxPageSetupDialogData& setup,
               double scale = 100,
               MaxPages shrinktofit = MaxPages::Unlimited,
               PrintLabels labels = Footer());

    virtual ~GraphPages() { }

    virtual void PreparePrinting();
    virtual bool PrintPage(int printoutPage, int graphPage);

    int GetPages() const { return m_pages.x * m_pages.y; }
    int GetRows() const { return m_pages.y; }
    int GetCols() const { return m_pages.x; }

    wxRect GetPrintRect() const { return m_print; }

    wxPrintout *GetPrintout() const { return m_printout; }
    void SetPrintout(wxPrintout *printout) { m_printout = printout; }

protected:
    virtual void DrawLabel(wxDC *dc, const PrintLabel& label,
                           const wxRect& rc, int page, int row, int col);

private:
    wxPrintout *m_printout;
    Graph *m_graph;
    double m_scale;
    MaxPages m_max;
    wxPageSetupDialogData m_setup;
    wxSize m_pages;
    wxPoint m_firstPage;
    wxRect m_print;
    wxRect m_header;
    wxRect m_footer;
    PrintLabels m_labels;
};

/**
 * @brief A wxPrintout class for Graph objects.
 *
 * @see Graph
 * @see MaxPages
 * @see PrintLabel
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
     *  Footer(_T("%PAGE% / %PAGES%"), wxALIGN_RIGHT));
     * @endcode
     *
     * @param graph The graph to print.
     * @param setup The setup data from the print setup dialog.
     * @param scale The maximum scaling percentage that should be used.
     * @param shrinktofit The maximum number of pages that should be used.
     * @param labels One or more headers and footers.
     * @param title Text displayed in the printing dialog.
     */
    GraphPrintout(Graph *graph,
                  const wxPageSetupDialogData& setup,
                  double scale = 100,
                  MaxPages shrinktofit = MaxPages::Unlimited,
                  PrintLabels labels = Footer(),
                  const wxString& title = _("Graph"));
    GraphPrintout(GraphPages *graphpages,
                  const wxString& title = _("Graph"));
    ~GraphPrintout();

    void OnPreparePrinting();
    bool HasPage(int page);
    bool OnPrintPage(int page);
    void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo);

private:
    GraphPages *m_graphpages;
};

} // namespace tt_solutions

#endif // GRAPHPRINT_H
