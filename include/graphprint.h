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

struct Margins
{
    Margins(int left, int right, int top, int bottom)
     : left(left), right(right), top(top), bottom(bottom)
    { }

    Margins(const wxPageSetupDialogData& pageSetup)
     : left(pageSetup.GetMarginTopLeft().x),
       right(pageSetup.GetMarginBottomRight().x),
       top(pageSetup.GetMarginTopLeft().y),
       bottom(pageSetup.GetMarginBottomRight().y)
    { }

    int left;
    int right;
    int top;
    int bottom;
};

struct MaxPages
{
    enum { Unlimited = 0 };

    MaxPages(int total = Unlimited)
     : rows(Unlimited), cols(Unlimited), total(total)
    { }

    MaxPages(int rows, int cols, int total = Unlimited)
     : rows(rows), cols(cols), total(total)
    { }

    int rows;
    int cols;
    int total;
};

class GraphPrintout : public wxPrintout
{
public:
    GraphPrintout(Graph *graph,
                  const Margins& margins,
                  double scale = 100,
                  MaxPages shrinktofit = MaxPages::Unlimited,
                  const wxString& title = _T("Graph"));

    void OnPreparePrinting();
    bool HasPage(int page);
    bool OnPrintPage(int page);
    void GetPageInfo(int *minPage, int *maxPage, int *pageFrom, int *pageTo);

private:
    Graph *m_graph;
    double m_scale;
    MaxPages m_maxpages;
    Margins m_margins;
    wxSize m_pages;
    wxPoint m_firstPage;
    wxRect m_print;
};

} // namespace tt_solutions

#endif // GRAPHPRINT_H
