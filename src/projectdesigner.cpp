/////////////////////////////////////////////////////////////////////////////
// Name:        projectdesigner.cpp
// Purpose:     Classes for laying out project graphs
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "projectdesigner.h"

namespace datactics {

using tt_solutions::GraphCtrl;
using std::max;

// ----------------------------------------------------------------------------
// ProjectDesigner
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(ProjectDesigner, GraphCtrl)

BEGIN_EVENT_TABLE(ProjectDesigner, GraphCtrl)
END_EVENT_TABLE()

const wxChar ProjectDesigner::DefaultName[] = _T("project_designer");

ProjectDesigner::ProjectDesigner()
{
    Init();
}

ProjectDesigner::ProjectDesigner(
        wxWindow *parent,
        wxWindowID id,
        const wxPoint& pos,
        const wxSize& size,
        long style,
        const wxValidator& validator,
        const wxString& name)
  : GraphCtrl(parent, id, pos, size, style, validator, name)
{
    Init();
}

void ProjectDesigner::Init()
{
    wxWindow *canvas = GetCanvas();

    canvas->SetBackgroundStyle(wxBG_STYLE_CUSTOM);
    canvas->Connect(wxEVT_ERASE_BACKGROUND,
                    wxEraseEventHandler(ProjectDesigner::OnCanvasBackground),
                    NULL, this);
}

ProjectDesigner::~ProjectDesigner()
{
}

void ProjectDesigner::OnCanvasBackground(wxEraseEvent& event)
{
    wxDC *pdc = event.GetDC();

    if (pdc) {
        DrawCanvasBackground(*pdc);
    }
    else {
        wxClientDC dc(GetCanvas());
        DrawCanvasBackground(dc);
    }
}

void ProjectDesigner::DrawCanvasBackground(wxDC& dc)
{
    wxRect rcClip = GetClientRect();
    //dc.GetClippingBox(rcClip);

    GetCanvas()->PrepareDC(dc);

    rcClip.x = dc.DeviceToLogicalX(rcClip.x);
    rcClip.y = dc.DeviceToLogicalY(rcClip.y);
    rcClip.SetRight(dc.DeviceToLogicalX(rcClip.GetRight()));
    rcClip.SetBottom(dc.DeviceToLogicalY(rcClip.GetBottom()));
    rcClip.Inflate(0, 1);

    const int factor = 5;
    int spacing = factor * GetGraph()->GetGridSpacing();

    wxRect rc = rcClip;
    int i = rc.x / spacing;
    rc.x = i * spacing;
    rc.width = spacing + 1;

    dc.SetPen(GetForegroundColour());
    wxColour colour = GetBackgroundColour();

    int red = colour.Red();
    int green = colour.Green();
    int blue = colour.Blue();

    int imax = (255 + factor - 1) / factor;
    if (i > imax)
        dc.SetBrush(colour);

    while (rc.x < rcClip.GetRight())
    {
        if (i == imax) {
            dc.SetBrush(colour);
        }
        else if (i < imax) {
            dc.SetBrush(wxColour(
                255 - (255 - red) * factor * i / 255,
                255 - (255 - green) * factor * i / 255,
                255 - (255 - blue) * factor * i / 255));
        }

        dc.DrawRectangle(rc);
        rc.x += spacing;
        i++;
    }

    wxCoord x1 = rcClip.x;
    wxCoord x2 = rcClip.GetRight();
    wxCoord y1 = rcClip.y + spacing - 1;
    y1 -= y1 % spacing;
    wxCoord y2 = rcClip.GetBottom();
    y2 -= y2 % spacing;

    while (y1 <= y2) {
        dc.DrawLine(x1, y1, x2, y1);
        y1 += spacing;
    }
}

// ----------------------------------------------------------------------------
// ProjectNode
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(ProjectNode, GraphNode)

ProjectNode::ProjectNode()
{
    m_borderThickness = 6;
    m_cornerRadius = 10;
    m_divide = 0;
}


ProjectNode::~ProjectNode()
{
}

void ProjectNode::SetId(const wxString& text)
{
    m_id = text;
}

void ProjectNode::SetResult(const wxString& text)
{
    m_result = text;
    Refresh();
}

void ProjectNode::SetIcon(const wxIcon& icon)
{
    m_icon = icon;
    Refresh();
}

void ProjectNode::OnSize(int& x, int& y)
{
    if (x < m_minSize.x)
        x = m_minSize.x;
    if (y < m_minSize.y)
        y = m_minSize.y;
}

void ProjectNode::OnLayout(wxDC &dc)
{
    int spacing = m_cornerRadius + m_borderThickness / 2 -
                  (m_cornerRadius - m_borderThickness / 2)
                  * 1000000 / 1414214;

    if (m_rcText.IsEmpty()) {
        wxCoord h, w;
        dc.GetMultiLineTextExtent(GetText(), &w, &h);
        m_rcText = wxRect(spacing, spacing, w, h);
    }

    int iconHSpace = 0;

    if (m_icon.Ok() && m_rcIcon.IsEmpty()) {
        m_rcIcon = wxRect(spacing, 0, m_icon.GetWidth(), m_icon.GetHeight());
        iconHSpace = m_rcIcon.width + spacing;
    }

    if (m_rcResult.IsEmpty()) {
        wxCoord h, w;
        dc.GetMultiLineTextExtent(GetResult(), &w, &h);
        m_rcResult = wxRect(spacing + iconHSpace, 0, w, h);
    }
    
    m_minSize.x = max(m_rcText.GetRight(), m_rcResult.GetRight()) +
                  spacing + 1;

    m_minSize.y = max(m_rcIcon.GetHeight(), m_rcResult.GetHeight()) +
                  m_rcText.GetBottom() + 2 + 2 * spacing - m_borderThickness;

    wxRect bounds = GetBounds();

    if (bounds.width < m_minSize.x || bounds.height < m_minSize.y) {
        bounds.width = m_minSize.x;
        bounds.height = m_minSize.y;
        SetSize(bounds.GetSize());
    }

    m_divide = m_rcText.GetBottom() + 1 + spacing - m_borderThickness;
    int mid = (m_divide + bounds.height) / 2;
    m_rcIcon.y = mid - m_rcIcon.height / 2;
    m_rcResult.y = mid - m_rcResult.height / 2;
}

void ProjectNode::OnDraw(wxDC& dc)
{
    OnLayout(dc);

    wxRect bounds = GetBounds();
    wxRect rc = bounds;
    rc.Deflate(m_borderThickness / 2);
    
    wxPen pen(GetColour(), m_borderThickness);
    wxBrush brush(GetBackgroundColour());

    dc.SetPen(pen);
    dc.SetBrush(brush);

    dc.DrawRoundedRectangle(rc, GetCornerRadius());
    rc.height = m_divide;
    dc.SetBrush(GetColour());
    dc.DrawRoundedRectangle(rc, GetCornerRadius());
    if (m_cornerRadius > m_borderThickness) {
        rc.y += m_cornerRadius;
        rc.height -= m_cornerRadius;
        dc.DrawRectangle(rc);
    }

    dc.DrawText(GetText(), bounds.GetTopLeft() + m_rcText.GetTopLeft());
    dc.DrawText(GetResult(), bounds.GetTopLeft() + m_rcResult.GetTopLeft());
    if (GetIcon().Ok())
        dc.DrawIcon(GetIcon(), bounds.GetTopLeft() + m_rcIcon.GetTopLeft());
}

} // namespace datactics
