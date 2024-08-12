/////////////////////////////////////////////////////////////////////////////
// Name:        graphprint.cpp
// Purpose:     Printout for the graph control
// Author:      Mike Wetherell
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <wx/dcprint.h>

#include "graphprint.h"

#undef min
#undef max

using std::min;
using std::max;

/**
 * @file
 * @brief Printing support.
 *
 * This file implements GraphPrintout and GraphPages classes.
 */

namespace tt_solutions {

// ----------------------------------------------------------------------------
// GraphPrintout
// ----------------------------------------------------------------------------

GraphPrintout::GraphPrintout(Graph *graph,
                             const wxPageSetupDialogData& setup,
                             double scale,
                             MaxPages shrinktofit,
                             PrintLabels labels,
                             double posX,
                             double posY,
                             const wxString& title)
  : wxPrintout(title),
    m_graphpages(new GraphPages(graph, setup, scale,
                                shrinktofit, std::move(labels), posX, posY))
{
    m_graphpages->SetPrintout(this);
}

GraphPrintout::GraphPrintout(GraphPages *graphpages,
                             const wxString& title)
  : wxPrintout(title),
    m_graphpages(graphpages)
{
    m_graphpages->SetPrintout(this);
}

GraphPrintout::~GraphPrintout()
{
    delete m_graphpages;
}

void GraphPrintout::OnPreparePrinting()
{
    m_graphpages->PreparePrinting();
}

bool GraphPrintout::HasPage(int page)
{
    return page >= 1 && page <= m_graphpages->GetPages();
}

bool GraphPrintout::OnPrintPage(int page)
{
    return HasPage(page) && m_graphpages->PrintPage(page, page);
}

void GraphPrintout::GetPageInfo(int *minPage,
                                int *maxPage,
                                int *pageFrom,
                                int *pageTo)
{
    *minPage = 1;
    *maxPage = m_graphpages->GetPages();
    *pageFrom = 1;
    *pageTo = *maxPage;
}

// ----------------------------------------------------------------------------
// GraphPages
// ----------------------------------------------------------------------------

constexpr double MAX_SCALE = 100;

GraphPages::GraphPages(Graph *graph,
                       const wxPageSetupDialogData& setup,
                       double scale,
                       MaxPages shrinktofit,
                       PrintLabels labels,
                       double posX,
                       double posY)
  : m_printout(NULL),
    m_graph(graph),
    m_scale(scale / MAX_SCALE),
    m_max(shrinktofit),
    m_setup(setup),
    m_labels(std::move(labels)),
    m_posX(min(max(posX, 0.0), 1.0)),
    m_posY(min(max(posY, 0.0), 1.0))
{
}

void GraphPages::PreparePrinting()
{
    wxASSERT(m_printout);
    wxASSERT(m_graph);

    m_graph->UnselectAll();

    int xdpi, ydpi;
    m_printout->GetPPIPrinter(&xdpi, &ydpi);

    wxSize dpiGraph = m_graph->GetDPI();

    // convert the margins into printer pixels
    int left   = MM::To<Pixels>(m_setup.GetMarginTopLeft().x, xdpi);
    int right  = MM::To<Pixels>(m_setup.GetMarginBottomRight().x, xdpi);
    int top    = MM::To<Pixels>(m_setup.GetMarginTopLeft().y, ydpi);
    int bottom = MM::To<Pixels>(m_setup.GetMarginBottomRight().y, ydpi);

    // the printing rect is then the paper size less the margins
    m_print = m_printout->GetPaperRectPixels();
    m_print.x += left;
    m_print.y += top;
    m_print.width -= left + right;
    m_print.height -= top + bottom;

    // intersect this with the printer's printable area
    int wprintable, hprintable;
    m_printout->GetPageSizePixels(&wprintable, &hprintable);
    m_print.Intersect(wxRect(0, 0, wprintable, hprintable));

    // calculate the height of the page headers and footers
    int header = 0, footer = 0;
    PrintLabels::iterator it;

    for (it = m_labels.begin(); it != m_labels.end(); ++it) {
        int height = MM::To<Pixels>(it->GetHeight(), ydpi);
        int position = it->GetPosition();

        if (position == wxTOP)
            header = max(header, height);
        else if (position == wxBOTTOM)
            footer = max(footer, height);

        wxFont font = it->GetFont();
        font.SetPointSize(font.GetPointSize() * ydpi / dpiGraph.y);
        it->SetFont(font);
    }

    m_header = m_print;
    m_print.y += header;
    m_print.height -= header + footer;
    m_header.height = header;
    m_footer = m_print;
    m_footer.y += m_print.height;
    m_footer.height = footer;

    // size of graph in inches
    wxRect rcGraph = m_graph->GetBounds();
    double wGraph = double(rcGraph.width) / dpiGraph.x;
    double hGraph = double(rcGraph.height) / dpiGraph.y;

    // size of printing area in inches
    double wPrint = double(m_print.width) / xdpi;
    double hPrint = double(m_print.height) / ydpi;

    if (m_max.pages)
        m_scale = min(m_scale, m_max.pages * wPrint * hPrint / (wGraph * hGraph));
    if (m_max.cols)
        m_scale = min(m_scale, m_max.cols * wPrint / wGraph);
    if (m_max.rows)
        m_scale = min(m_scale, m_max.rows * hPrint / hGraph);

    for (;;) {
        // work out how many pages it would take at this scaling
        m_pages.x = wxMax(int(ceil(wGraph * m_scale / wPrint)), 1);
        m_pages.y = wxMax(int(ceil(hGraph * m_scale / hPrint)), 1);

        // satisfies the max pages?
        if (m_max.pages <= 0 || m_pages.x * m_pages.y <= m_max.pages)
            break;

        double sx = (m_pages.x - 1) * wPrint / wGraph;
        double sy = (m_pages.y - 1) * hPrint / hGraph;

        if (sy == 0 || (sx != 0 && sx < sy))
            m_scale = sx;
        else
            m_scale = sy;
    }

    double w = wPrint / m_scale * dpiGraph.x;
    double h = hPrint / m_scale * dpiGraph.y;

    m_firstPage.x = rcGraph.x + int((rcGraph.width - w * m_pages.x) * m_posX);
    m_firstPage.y = rcGraph.y + int((rcGraph.height - h * m_pages.y) * m_posY);
}

void GraphPages::DrawLabel(wxDC *dc, const PrintLabel& label,
                           const wxRect& rc, int page, int row, int col)
{
    int min, max, from, to;
    GetPrintout()->GetPageInfo(&min, &max, &from, &to);

    wxString text = label.GetText();
    text.Replace(_T("%ROW%"),   wxString() << row);
    text.Replace(_T("%ROWS%"),  wxString() << GetRows());
    text.Replace(_T("%COL%"),   wxString() << col);
    text.Replace(_T("%COLS%"),  wxString() << GetCols());
    text.Replace(_T("%PAGE%"),  wxString() << page);
    text.Replace(_T("%PAGES%"), wxString() << max);

    dc->SetFont(label.GetFont());
    dc->DrawLabel(text, rc, label.GetAlignment());
}

bool GraphPages::PrintPage(int printoutPage, int graphPage)
{
    wxDC *dc = m_printout->GetDC();
    if (!dc || graphPage < 1)
        return false;

    --graphPage;
    int xpage = graphPage % m_pages.x;
    int ypage = graphPage / m_pages.x;

    int wpage, hpage;
    m_printout->GetPageSizePixels(&wpage, &hpage);

    int wdc, hdc;
    dc->GetSize(&wdc, &hdc);

    int xdpi, ydpi;
    m_printout->GetPPIPrinter(&xdpi, &ydpi);

    dc->SetUserScale(double(wdc) / wpage, double(hdc) / hpage);
    dc->SetDeviceOrigin(0, 0);
    dc->SetLogicalOrigin(0, 0);

    PrintLabels::const_iterator it;

    for (it = m_labels.begin(); it != m_labels.end(); ++it) {
        int position = it->GetPosition();
        wxRect rc;

        if (position == wxTOP)
            rc = m_header;
        else if (position == wxBOTTOM)
            rc = m_footer;

        DrawLabel(dc, *it, rc, printoutPage, ypage + 1, xpage + 1);
    }

    wxSize dpiGraph = m_graph->GetDPI();

    dc->SetUserScale(
        xdpi * m_scale * wdc / (dpiGraph.x * wpage),
        ydpi * m_scale * hdc / (dpiGraph.y * hpage));

    dc->SetDeviceOrigin(
        (m_print.x - m_print.width * xpage) * wdc / wpage,
        (m_print.y - m_print.height * ypage) * hdc / hpage);

    constexpr int ARBITRARY_POSITIVE_COORD = 100;
    int signX = dc->LogicalToDeviceX(ARBITRARY_POSITIVE_COORD) >= dc->LogicalToDeviceX(0) ? 1 : -1;
    int signY = dc->LogicalToDeviceY(ARBITRARY_POSITIVE_COORD) >= dc->LogicalToDeviceY(0) ? 1 : -1;

    dc->SetLogicalOrigin(signX * m_firstPage.x, signY * m_firstPage.y);

    double x1 = m_print.width * xpage * dpiGraph.x / m_scale / xdpi;
    double y1 = m_print.height * ypage * dpiGraph.y / m_scale / ydpi;
    double x2 = m_print.width * (xpage + 1) * dpiGraph.x / m_scale / xdpi;
    double y2 = m_print.height * (ypage + 1) * dpiGraph.y / m_scale / ydpi;

    wxRect rcPage(
        m_firstPage.x + int(x1),
        m_firstPage.y + int(y1),
        int(ceil(x2)) - int(x1),
        int(ceil(y2)) - int(y1));

    m_graph->Draw(dc, rcPage);

    return true;
}

} // namespace tt_solutions
