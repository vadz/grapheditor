/////////////////////////////////////////////////////////////////////////////
// Name:        graphprint.cpp
// Purpose:     Printout for the graph control
// Author:      Mike Wetherell
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "graphprint.h"

#undef min
using std::min;

namespace tt_solutions {

GraphPrintout::GraphPrintout(Graph *graph,
                             const Margins& margins,
                             double scale,
                             MaxPages shrinktofit,
                             const wxString& title)
  : wxPrintout(title),
    m_graph(graph),
    m_scale(scale / 100),
    m_max(shrinktofit),
    m_margins(margins)
{
    graph->UnselectAll();
}

void GraphPrintout::OnPreparePrinting()
{
    int xdpi, ydpi;
    GetPPIPrinter(&xdpi, &ydpi);

    // convert the margins into printer pixels
    int left   = MM::To<Pixels>(m_margins.left, xdpi);
    int right  = MM::To<Pixels>(m_margins.right, xdpi);
    int top    = MM::To<Pixels>(m_margins.top, ydpi);
    int bottom = MM::To<Pixels>(m_margins.bottom, ydpi);

    // the printing rect is then the paper size less the margins
    m_print = GetPaperRectPixels();
    m_print.x += left;
    m_print.y += top;
    m_print.width -= left + right;
    m_print.height -= top + bottom;

    // intersect this with the printer's printable area
    int wprintable, hprintable;
    GetPageSizePixels(&wprintable, &hprintable);
    m_print.Intersect(wxRect(0, 0, wprintable, hprintable));

    // size of graph in inches
    wxRect rcGraph = m_graph->GetBounds();
    wxSize dpiGraph = m_graph->GetDPI();
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

    m_firstPage.x = rcGraph.x + int((rcGraph.width - w * m_pages.x) / 2);
    m_firstPage.y = rcGraph.y + int((rcGraph.height - h * m_pages.y) / 2);
}

bool GraphPrintout::HasPage(int page)
{
    return page > 0 && page <= m_pages.x * m_pages.y;
}

bool GraphPrintout::OnPrintPage(int page)
{
    wxDC *dc = GetDC();
    if (!dc || !HasPage(page))
        return false;

    page--;
    int xpage = page % m_pages.x;
    int ypage = page / m_pages.x;

    int wpage, hpage;
    GetPageSizePixels(&wpage, &hpage);

    int wdc, hdc;
    dc->GetSize(&wdc, &hdc);

    int xdpi, ydpi;
    GetPPIPrinter(&xdpi, &ydpi);

    wxSize dpiGraph = m_graph->GetDPI();

    dc->SetUserScale(
        xdpi * m_scale * wdc / (dpiGraph.x * wpage),
        ydpi * m_scale * hdc / (dpiGraph.y * hpage));

    dc->SetDeviceOrigin(
        (m_print.x - m_print.width * xpage) * wdc / wpage,
        (m_print.y - m_print.height * ypage) * hdc / hpage);

    int signX = dc->LogicalToDeviceX(100) >= dc->LogicalToDeviceX(0) ? 1 : -1;
    int signY = dc->LogicalToDeviceY(100) >= dc->LogicalToDeviceY(0) ? 1 : -1;

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

void GraphPrintout::GetPageInfo(int *minPage,
                                int *maxPage,
                                int *pageFrom,
                                int *pageTo)
{
    *minPage = 1;
    *maxPage = m_pages.x * m_pages.y;
    *pageFrom = 1;
    *pageTo = *maxPage;
}

} // namespace tt_solutions
