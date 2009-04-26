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

namespace tt_solutions {

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

class PrintLabel
{
public:
    PrintLabel(const wxString& text, int flags, int height, wxFont font)
      : m_text(text), m_flags(flags), m_height(height), m_font(font)
    { }

    void SetText(const wxString& text) { m_text = text; }
    wxString GetText() const { return m_text; }

    void SetFont(const wxFont& font) { m_font = font; }
    wxFont GetFont() const { return m_font; }

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

inline PrintLabels Footer(
    const wxString& text = _T("Page %PAGE% of %PAGES%"),
    int align = wxALIGN_CENTRE,
    int height = 12,
    const wxFont& font = wxFont(12, wxSWISS, wxNORMAL, wxNORMAL))
{
    return PrintLabel(text, align | wxALIGN_BOTTOM | wxBOTTOM, height, font);
}

inline PrintLabels Header(
    const wxString& text = _T("Page %PAGE% of %PAGES%"),
    int align = wxALIGN_CENTRE,
    int height = 12,
    const wxFont& font = wxFont(12, wxSWISS, wxNORMAL, wxNORMAL))
{
    return PrintLabel(text, align | wxALIGN_TOP | wxTOP, height, font);
}

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

class GraphPrintout : public wxPrintout
{
public:
    GraphPrintout(Graph *graph,
                  const wxPageSetupDialogData& setup,
                  double scale = 100,
                  MaxPages shrinktofit = MaxPages::Unlimited,
                  PrintLabels labels = Footer(),
                  const wxString& title = _T("Graph"));
    GraphPrintout(GraphPages *graphpages,
                  const wxString& title = _T("Graph"));
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
