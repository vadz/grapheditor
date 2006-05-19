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
#include <cstdlib>

namespace datactics {

using tt_solutions::GraphCtrl;
using std::min;
using std::max;
using std::abs;

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

    m_background[0] = m_background[1] = GetBackgroundColour();
    m_showGrid = true;
}

ProjectDesigner::~ProjectDesigner()
{
}

void ProjectDesigner::SetBackgroundGradient(const wxColour& from,
                                            const wxColour& to)
{
    m_background[0] = from;
    m_background[1] = to;
}

void ProjectDesigner::SetShowGrid(bool show)
{
    if (show != m_showGrid) {
        m_showGrid = show;
        GetCanvas()->Refresh();
    }
}

void ProjectDesigner::OnCanvasBackground(wxEraseEvent& event)
{
    if (GetGraph()) {
        wxDC *pdc = event.GetDC();

        if (pdc) {
            DrawCanvasBackground(*pdc);
        }
        else {
            wxClientDC dc(GetCanvas());
            DrawCanvasBackground(dc);
        }
    }
    else {
        event.Skip();
    }
}

void ProjectDesigner::DrawCanvasBackground(wxDC& dc)
{
    wxASSERT(GetGraph());
    wxWindow *canvas = GetCanvas();

    wxRect rcClip;
    dc.GetClippingBox(rcClip);
    rcClip.Inflate(1, 1);

    canvas->PrepareDC(dc);

    wxRect rc;
    rc.x = dc.DeviceToLogicalX(rcClip.x);
    rc.y = dc.DeviceToLogicalY(rcClip.y);
    rc.SetRight(dc.DeviceToLogicalX(rcClip.GetRight()));
    rc.SetBottom(dc.DeviceToLogicalY(rcClip.GetBottom()));
    rcClip = rc;

    int factor, spacing;

    if (IsGridShown()) {
        factor = 5;
        int zoom = GetZoom();

        if (zoom) {
            while (zoom <= 50) {
                factor *= 2;
                zoom *= 2;
            }
        }

        spacing = factor * GetGraph()->GetGridSpacing();
    }
    else {
        factor = 1;
        spacing = GetGraph()->GetGridSpacing();
    }

    rc.x -= rc.x % spacing;
    if (rcClip.x < 0)
        rc.x -= spacing;
    rc.width = spacing + 1;

    int lastred = -1, lastgreen = -1, lastblue = -1;
    int red0 = m_background[0].Red();
    int green0 = m_background[0].Green();
    int blue0 = m_background[0].Blue();
    int red1 = m_background[1].Red();
    int green1 = m_background[1].Green();
    int blue1 = m_background[1].Blue();

    dc.SetPen(*wxTRANSPARENT_PEN);

    while (rc.x < rcClip.GetRight())
    {
        int i = min(abs(rc.x / spacing) * factor, 255);

        int red = red0 + (red1 - red0) * i / 255;
        int green = green0 + (green1 - green0) * i / 255;
        int blue = blue0 + (blue1 - blue0) * i / 255;

        if (red != lastred || green != lastgreen || blue != lastblue) {
            dc.SetBrush(wxColour(red, green, blue));
            lastred = red;
            lastgreen = green;
            lastblue = blue;
        }

        dc.DrawRectangle(rc);
        rc.x += spacing;
    }

    if (IsGridShown()) {
        dc.SetPen(GetForegroundColour());
        wxCoord x1, y1, x2, y2;

        x1 = rcClip.x - rcClip.x % spacing;
        if (rcClip.x < 0)
            x1 -= spacing;
        x2 = rcClip.GetRight() - rcClip.GetRight() % spacing;
        if (rcClip.GetRight() > 0)
            x2 += spacing;
        y1 = rcClip.y;
        y2 = rcClip.GetBottom();

        while (x1 <= x2) {
            dc.DrawLine(x1, y1, x1, y2);
            x1 += spacing;
        }

        x1 = rcClip.x;
        x2 = rcClip.GetRight();
        y1 = rcClip.y - rcClip.y % spacing;
        if (rcClip.y < 0)
            y1 -= spacing;
        y2 = rcClip.GetBottom() - rcClip.GetBottom() % spacing;
        if (rcClip.GetBottom() > 0)
            y2 += spacing;

        while (y1 <= y2) {
            dc.DrawLine(x1, y1, x2, y1);
            y1 += spacing;
        }
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
    SetStyle(Style_Custom);
}


ProjectNode::~ProjectNode()
{
}

void ProjectNode::SetText(const wxString& text)
{
    m_rcText = wxRect();
    GraphNode::SetText(text);
}

void ProjectNode::SetFont(const wxFont& font)
{
    m_rcText = wxRect();
    m_rcResult = wxRect();
    GraphNode::SetFont(font);
}

void ProjectNode::SetId(const wxString& text)
{
    m_id = text;
}

void ProjectNode::SetResult(const wxString& text)
{
    m_result = text;
    m_rcResult = wxRect();
    Layout();
    Refresh();
}

void ProjectNode::SetIcon(const wxIcon& icon)
{
    m_icon = icon;
    Layout();
    Refresh();
}

void ProjectNode::SetBorderThickness(int thickness)
{
    m_borderThickness = thickness;
    Layout();
    Refresh();
}

void ProjectNode::SetCornerRadius(int radius)
{
    m_cornerRadius = radius;
    Layout();
    Refresh();
}

int ProjectNode::HitTest(const wxPoint& pt) const
{
    wxRect bounds = GetBounds();

    if (!bounds.Inside(pt))
        return Hit_No;

    // for the custom style detect operation, result or icon
    if (GetStyle() == Style_Custom) {
        wxPoint ptNode = pt - bounds.GetTopLeft();

        if (m_rcText.Inside(ptNode))
            return Hit_Operation;
        if (m_rcResult.Inside(ptNode))
            return Hit_Result;
        if (m_rcIcon.Inside(ptNode))
            return Hit_Image;
    }

    // for non-custom sytles it just returns yes or no
    return Hit_Yes;
}

// Recalculates the positions of the things within the node. Only recalcs
// the sizes of the text lables, m_rcText and m_rcResult, when the rects
// are null, so usual it runs quickly.
//
void ProjectNode::OnLayout(wxDC &dc)
{
    int spacing = 0;

    // this keeps the text within the inner radius of the curved corners
    if (m_cornerRadius > m_borderThickness)
        spacing = m_cornerRadius + m_borderThickness / 2 -
                  (m_cornerRadius - m_borderThickness / 2)
                  * 1000000 / 1414214;
    // if the border is thicker than the corner curve then just allow 3
    if (spacing < m_borderThickness + 3)
        spacing = m_borderThickness + 3;

    if (m_rcText.IsEmpty() || m_rcResult.IsEmpty())
        dc.SetFont(GetFont());

    // figure out the bounds of the top text label
    if (m_rcText.IsEmpty()) {
        wxCoord h, w;
        dc.GetMultiLineTextExtent(GetText(), &w, &h);
        m_rcText.width = w;
        m_rcText.height = h;
    }
    m_rcText.x = spacing;
    m_rcText.y = spacing;

    // bounds of the icon without worrying about it's y position
    if (m_icon.Ok()) {
        m_rcIcon.width = m_icon.GetWidth();
        m_rcIcon.height = m_icon.GetHeight();
    } else {
        m_rcIcon.width = 0;
        m_rcIcon.height = 0;
    }
    m_rcIcon.x = spacing;

    int iconHSpace = m_rcIcon.width + spacing;

    // bounds of the lower text, without calculating the y position
    if (m_rcResult.IsEmpty()) {
        wxCoord h, w;
        dc.GetMultiLineTextExtent(GetResult(), &w, &h);
        m_rcResult.width = w;
        m_rcResult.height = h;
    }
    m_rcResult.x = spacing + iconHSpace;

    // calcuate the position of the dividing line between the two sections
    m_divide = m_rcText.GetBottom() + 1 + spacing - m_borderThickness;
    m_divide = max(m_divide, m_cornerRadius + m_borderThickness / 2);

    // calculate the min size the node must have to fit everything
    m_minSize.x = max(m_rcText.GetRight(), m_rcResult.GetRight()) +
                  spacing + 1;
    m_minSize.x = max(m_minSize.x, 2 * m_cornerRadius + m_borderThickness);
    m_minSize.y = max(m_rcIcon.GetHeight(), m_rcResult.GetHeight()) +
                  m_rcText.GetBottom() + 2 + 2 * spacing;
    m_minSize.y = max(m_minSize.y, m_divide + m_cornerRadius + m_borderThickness / 2);

    wxRect bounds = GetBounds();

    // resize the node if it's too small
    if (bounds.width < m_minSize.x || bounds.height < m_minSize.y) {
        if (bounds.width < m_minSize.x)
            bounds.width = m_minSize.x;
        if (bounds.height < m_minSize.y)
            bounds.height = m_minSize.y;
        SetSize(bounds.GetSize());
    }

    // vertically centre the icon and result text in the lower section
    int mid = (m_divide + bounds.height) / 2;
    m_rcIcon.y = mid - m_rcIcon.height / 2;
    m_rcResult.y = mid - m_rcResult.height / 2;
}

void ProjectNode::OnDraw(wxDC& dc)
{
    if (GetStyle() == Style_Custom) {
        wxRect bounds = GetBounds();
        wxRect rc = bounds;
        // deflate by half the border thickness so that the whole stays
        // within the bounds
        rc.Deflate(m_borderThickness / 2);

        dc.SetPen(wxPen(GetColour(), m_borderThickness));
        dc.SetBrush(GetBackgroundColour());
        dc.SetFont(GetFont());
        dc.SetTextForeground(GetTextColour());

        // border, filled with bg colour
        dc.DrawRoundedRectangle(rc, GetCornerRadius());

        // fill top section with the border color
        dc.SetBrush(GetColour());
        int r = m_cornerRadius;
        int x1 = rc.x, x2 = rc.GetRight();
        dc.DrawArc(x1 + r, rc.y, x1, rc.y + r, x1 + r, rc.y + r);
        dc.DrawArc(x2, rc.y + r, x2 - r, rc.y, x2 - r, rc.y + r);
        dc.DrawRectangle(x1 + r, rc.y, x2 - x1 - 2 * r, r);
        dc.DrawRectangle(x1, rc.y + r, x2 - x1, m_divide - r);

        // upper text
        rc = m_rcText;
        rc.Offset(bounds.GetTopLeft());
        dc.DrawLabel(GetText(), rc);

        // lower text
        rc = m_rcResult;
        rc.Offset(bounds.GetTopLeft());
        dc.DrawLabel(GetResult(), rc);

        // icon
        if (GetIcon().Ok())
            dc.DrawIcon(GetIcon(), bounds.GetTopLeft() +
                                   m_rcIcon.GetTopLeft());
    }
    else {
        GraphNode::OnDraw(dc);
    }
}

wxPoint ProjectNode::GetCornerPoint(const wxPoint& centre,
                                    int radius, int sign,
                                    const wxPoint& inside,
                                    const wxPoint& outside) const
{
    wxPoint k = inside, pt = outside;
    radius++;

    // translate the line, so that the centre of the circle is at the origin
    // so the circle is now x^2 + y^2 = radius^2
    k -= centre;
    pt -= centre;

    // work out m and c for the line y = m x + c through the points
    double m = pt.y - k.y;
    m /= pt.x - k.x;
    double c = pt.y - m * pt.x;

    // work out some squares ready
    double r2 = radius * radius;
    double m2 = m * m;
    double c2 = c * c;

    double G = sqrt((m2 + 1) * r2 - c2);

    // this is the solution of y = m x + c and y^2 + x^2 = radius^2, there
    // are two solutions and sign should be +1 or -1 to select between them.
    double x = (sign * G - c * m) / (m2 + 1);
    double y = (sign * G * m + c) / (m2 + 1);

    return centre + wxPoint(int(x), int(y));
}

wxPoint ProjectNode::GetPerimeterPoint(const wxPoint& inside,
                                       const wxPoint& outside) const
{
    wxPoint pt = GraphNode::GetPerimeterPoint(inside, outside);

    wxRect b = GetBounds();
    int r = m_cornerRadius + m_borderThickness / 2;

    // deflate so that the corners are the centres of the corner circles
    b.Deflate(r);

    // avoid cases GetCornerPoint won't handle
    if (b.IsEmpty() || inside.x == outside.x || inside.y == outside.y)
        return pt;

    // check if in a corner
    if (pt.x < b.x && pt.y < b.y)
        pt = GetCornerPoint(b.GetTopLeft(), r, -1, inside, outside);

    else if (pt.x > b.GetRight() && pt.y < b.y)
        pt = GetCornerPoint(wxPoint(b.GetRight(), b.y), r, 1, inside, outside);

    else if (pt.x < b.x && pt.y > b.GetBottom())
        pt = GetCornerPoint(wxPoint(b.x, b.GetBottom()), r, -1, inside, outside);

    else if (pt.x > b.GetRight() && pt.y > b.GetBottom())
        pt = GetCornerPoint(b.GetBottomRight(), r, 1, inside, outside);

    return pt;
}

} // namespace datactics
