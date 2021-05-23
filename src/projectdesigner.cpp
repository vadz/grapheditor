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

/**
 * @file
 * @brief Support for laying out project graphs.
 */

namespace datactics {

using namespace tt_solutions;

#undef min
#undef max

using std::min;
using std::max;
using std::abs;

// ----------------------------------------------------------------------------
// ProjectDesigner
// ----------------------------------------------------------------------------

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

    canvas->Bind(wxEVT_ERASE_BACKGROUND,
                 &ProjectDesigner::OnCanvasBackground, this);

    m_background[0] = m_background[1] = GetBackgroundColour();
    m_showGrid = true;
    m_gridFactor = 5;
}

ProjectDesigner::~ProjectDesigner()
{
}

bool ProjectDesigner::SetBackgroundColour(const wxColour& colour)
{
    m_background[0] = m_background[1] = colour;
    return true;
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

int ProjectDesigner::AdjustedGridFactor() const
{
    int factor = max(m_gridFactor, 1);
    double zoom = GetZoom();

    if (zoom > 0) {
        while (zoom <= 50) {
            factor *= 2;
            zoom *= 2;
        }
    }

    return factor;
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

    wxSize spacing = GetGraph()->GetGridSpacing();
    int factor;

    if (IsGridShown()) {
        factor = AdjustedGridFactor();
        spacing *= factor;
    }
    else {
        factor = 1;
    }

    rc.x -= rc.x % spacing.x;
    if (rcClip.x < 0)
        rc.x -= spacing.x;
    rc.width = spacing.x + 1;

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
        int i = min(abs(rc.x / spacing.x) * factor, 255);

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
        rc.x += spacing.x;
    }

    if (IsGridShown()) {
        dc.SetPen(GetForegroundColour());
        wxCoord x1, y1, x2, y2;

        x1 = rcClip.x - rcClip.x % spacing.x;
        if (rcClip.x < 0)
            x1 -= spacing.x;
        x2 = rcClip.GetRight() - rcClip.GetRight() % spacing.x;
        if (rcClip.GetRight() > 0)
            x2 += spacing.x;
        y1 = rcClip.y;
        y2 = rcClip.GetBottom();

        while (x1 <= x2) {
            dc.DrawLine(x1, y1, x1, y2);
            x1 += spacing.x;
        }

        x1 = rcClip.x;
        x2 = rcClip.GetRight();
        y1 = rcClip.y - rcClip.y % spacing.y;
        if (rcClip.y < 0)
            y1 -= spacing.y;
        y2 = rcClip.GetBottom() - rcClip.GetBottom() % spacing.y;
        if (rcClip.GetBottom() > 0)
            y2 += spacing.y;

        while (y1 <= y2) {
            dc.DrawLine(x1, y1, x2, y1);
            y1 += spacing.y;
        }
    }
}

// ----------------------------------------------------------------------------
// ProjectNode
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(ProjectNode, GraphNode)

ProjectNode::ProjectNode(const wxString& operation,
                         const wxString& result,
                         const wxString& id,
                         const wxIcon& icon,
                         const wxColour& colour,
                         const wxColour& bgcolour,
                         const wxColour& textcolour)
  : GraphNode(operation, colour, bgcolour, textcolour),
    m_id(id),
    m_result(result),
    m_icon(icon),
    m_maxAutoSize(Pixels::From<Points>(wxSize(144, 72), GetDPI()))
{
    m_borderThickness = 90;
    m_cornerRadius = 150;
    m_divide = 0;
    SetStyle(Style_Custom);
}

ProjectNode::~ProjectNode()
{
}

void ProjectNode::SetText(const wxString& text)
{
    m_rcText = wxRect();
    SetToolTip(wxEmptyString);
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
    SetToolTip(wxEmptyString);
    Layout();
    Refresh();
}

void ProjectNode::SetIcon(const wxIcon& icon)
{
    m_icon = icon;
    Layout();
    Refresh();
}

int ProjectNode::HitTest(const wxPoint& pt) const
{
    wxRect bounds = GetBounds();

    if (!bounds.Contains(pt))
        return Hit_No;

    // for the custom style detect operation, result or icon
    if (GetStyle() == Style_Custom) {
        wxPoint ptNode = pt - bounds.GetTopLeft();

        if (m_rcText.Contains(ptNode))
            return Hit_Operation;
        if (m_rcResult.Contains(ptNode))
            return Hit_Result;
        if (m_rcIcon.Contains(ptNode))
            return Hit_Image;
    }

    // for non-custom styles it just returns yes or no
    return Hit_Yes;
}

int ProjectNode::GetSpacing() const
{
    int spacing = 0;
    int border = GetBorderThickness();
    int corner = GetCornerRadius();

    // this keeps the text within the inner radius of the curved corners
    if (corner > border)
        spacing = corner + border / 2 - (corner - border / 2)
                  * 1000000 / 1414214;
    // if the border is thicker than the corner curve then just allow 3
    if (spacing < border + 3)
        spacing = border + 3;

    return spacing;
}

// Recalculates the positions of the things within the node. Only recalcs
// the sizes of the text labels, m_rcText and m_rcResult, when the rects
// are null, so usual it runs quickly.
//
void ProjectNode::OnLayout(wxDC &dc)
{
    int spacing = GetSpacing();
    int border = GetBorderThickness();
    int corner = GetCornerRadius();

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

    // calculate the position of the dividing line between the two sections
    m_divide = m_rcText.GetBottom() + 1 + spacing - border;
    m_divide = max(m_divide, corner + border / 2);

    // calculate the size the node must have to fit everything
    wxSize fullSize;
    fullSize.x = max(m_rcText.GetRight(), m_rcResult.GetRight()) + spacing + 1;
    fullSize.x = max(fullSize.x, 2 * corner + border);
    fullSize.y = max(m_rcIcon.GetHeight(), m_rcResult.GetHeight()) +
                 m_rcText.GetBottom() + 2 + 2 * spacing;
    fullSize.y = max(fullSize.y, m_divide + corner + border / 2);

    // the minimum size
    wxSize minSize;
    minSize.x = min(fullSize.x, m_maxAutoSize.x);
    minSize.y = min(fullSize.y, m_maxAutoSize.y);

    wxSize size, orig = GetSize();

    // resize the node if it's too small
    size.x = max(orig.x, minSize.x);
    size.y = max(orig.y, minSize.y);
    if (size != orig)
        DoSetSize(dc, size);

    // vertically centre the icon and result text in the lower section
    int mid = (m_divide + size.y) / 2;
    m_rcIcon.y = max(m_divide + border, mid - m_rcIcon.height / 2);
    m_rcResult.y = max(m_divide + border, mid - m_rcResult.height / 2);

    // set the toolip if the text doesn't fit
    wxRect rcText(m_rcText), rcResult(m_rcResult), inner(size);
    inner.Deflate(spacing - 1);
    rcText.Intersect(inner);
    rcResult.Intersect(inner);

    bool needTip = rcText != m_rcText || rcResult != m_rcResult;
    bool haveTip = !GetToolTip().empty();

    if (needTip && !haveTip)
        SetToolTip(GetText() + _T("\n") + GetResult());
    else if (!needTip && haveTip)
        SetToolTip(wxEmptyString);
}

void ProjectNode::OnDraw(wxDC& dc)
{
    if (GetStyle() == Style_Custom) {
        wxRect bounds = GetBounds();
        wxRect clip = GetGraph()->GetDrawRect();

        if (clip.IsEmpty())
            clip = bounds;
        else if (!clip.Intersects(bounds))
            return;
        else
            dc.SetClippingRegion(clip);

        int border = GetBorderThickness();
        int corner = GetCornerRadius();

        wxRect rc = bounds;
        // deflate by half the border thickness so that the whole stays
        // within the bounds
        rc.Deflate(border / 2);

        dc.SetPen(wxPen(GetColour(), border));
        dc.SetBrush(GetBackgroundColour());
        dc.SetFont(GetFont());
        dc.SetTextForeground(GetTextColour());

        // border, filled with bg colour
        dc.DrawRoundedRectangle(rc, GetCornerRadius());

        // fill top section with the border color
        dc.SetBrush(GetColour());
        int r = corner;
        int x1 = rc.x, x2 = rc.GetRight();
        dc.DrawArc(x1 + r, rc.y, x1, rc.y + r, x1 + r, rc.y + r);
        dc.DrawArc(x2, rc.y + r, x2 - r, rc.y, x2 - r, rc.y + r);
        dc.DrawRectangle(x1 + r, rc.y, x2 - x1 - 2 * r, r);
        dc.DrawRectangle(x1, rc.y + r, x2 - x1, m_divide - r);

        wxRect inner(bounds);
        inner.Deflate(GetSpacing());
        wxDCClipper clipper(dc, inner.Intersect(clip));

        // upper text
        rc = m_rcText;
        rc.Offset(bounds.GetTopLeft());
        if (clip.Intersects(rc))
            dc.DrawLabel(GetText(), rc);

        // lower text
        rc = m_rcResult;
        rc.Offset(bounds.GetTopLeft());
        if (clip.Intersects(rc))
            dc.DrawLabel(GetResult(), rc);

        // icon
        if (GetIcon().Ok()) {
            rc = m_rcIcon;
            rc.Offset(bounds.GetTopLeft());
            if (clip.Intersects(rc)) {
                wxBitmap bmp(rc.width, rc.height);
                wxMemoryDC mdc(bmp);

                mdc.SetBrush(GetBackgroundColour());
                mdc.Clear();

                mdc.DrawIcon(GetIcon(), 0, 0);
                dc.Blit(rc.GetTopLeft(), rc.GetSize(), &mdc, wxPoint());
            }
        }
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
    int border = GetBorderThickness();
    int corner = GetCornerRadius();

    wxPoint pt = GraphNode::GetPerimeterPoint(inside, outside);

    wxRect b = GetBounds();
    int r = corner + border / 2;

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

bool ProjectNode::Serialise(Archive::Item& arc)
{
    const ProjectNode& def = Factory<ProjectNode>(this).GetDefault();
    arc.Exch(_T("result"), m_result, def.m_result);
    arc.Exch(_T("id"), m_id, def.m_id);
    arc.Exch(_T("borderthickness"), m_borderThickness, def.m_borderThickness);
    arc.Exch(_T("cornerradius"), m_cornerRadius, def.m_cornerRadius);
    arc.Exch(_T("icon"), m_icon, def.m_icon);
    arc.Exch(_T("maxautosize"), m_maxAutoSize, def.m_maxAutoSize);

    if (!GraphNode::Serialise(arc))
        return false;

    arc.Remove(_T("tooltip"));

    return true;
}

IMPLEMENT_DYNAMIC_CLASS(ProjectDesigner, GraphCtrl)

} // namespace datactics
