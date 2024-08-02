/////////////////////////////////////////////////////////////////////////////
// Name:        ogldiag.cpp
// Purpose:     wxDiagram
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#ifdef new
#undef new
#endif

#include <ctype.h>
#include <math.h>
#include <stdlib.h>

#include "wx/ogl/ogl.h"


IMPLEMENT_DYNAMIC_CLASS(wxDiagram, wxObject)

// Object canvas
wxDiagram::wxDiagram()
{
  m_diagramCanvas = nullptr;
  m_snapToGrid = true;
  m_gridSpacing = 5.0;
  m_gridSpacingAspect = 1.0;
  m_shapeList = new wxList;
  m_mouseTolerance = DEFAULT_MOUSE_TOLERANCE;
}

wxDiagram::~wxDiagram()
{
  if (m_shapeList)
    delete m_shapeList;
}

void wxDiagram::SetSnapToGrid(bool snap)
{
  m_snapToGrid = snap;
}

void wxDiagram::SetGridSpacing(double x, double y)
{
  m_gridSpacing = y;
  m_gridSpacingAspect = x / y;
}

void wxDiagram::GetGridSpacing(double *x, double *y) const
{
  *y = m_gridSpacing;
  *x = m_gridSpacing * m_gridSpacingAspect;
}

void wxDiagram::Snap(double *x, double *y)
{
  if (m_snapToGrid)
  {
    double xspacing = m_gridSpacing * m_gridSpacingAspect;
    *x = xspacing * ((int)(*x/xspacing + 0.5));
    *y = m_gridSpacing * ((int)(*y/m_gridSpacing + 0.5));
  }
}


void wxDiagram::Redraw(wxDC& dc)
{
  if (m_shapeList)
  {
    auto current = m_shapeList->GetFirst();

    while (current)
    {
      wxShape *object = (wxShape *)current->GetData();
      if (!object->GetParent())
        object->Draw(dc);

      current = current->GetNext();
    }
  }
}

void wxDiagram::Clear(wxDC& dc)
{
  dc.Clear();
}

// Insert object after addAfter, or at end of list.
void wxDiagram::AddShape(wxShape *object, wxShape *addAfter)
{
  if (m_shapeList->Member(object))
    return;

  object->SetCanvas(GetCanvas());

  if (addAfter)
  {
    const auto nodeAfter = m_shapeList->Find(addAfter);
    if (nodeAfter && nodeAfter->GetNext())
    {
      m_shapeList->Insert(nodeAfter->GetNext(), object);
      return;
    }
  }

  m_shapeList->Append(object);
}

void wxDiagram::InsertShape(wxShape *object)
{
  m_shapeList->Insert(object);
  object->SetCanvas(GetCanvas());
}

void wxDiagram::RemoveShape(wxShape *object)
{
  m_shapeList->DeleteObject(object);
}

// Should this delete the actual objects too? I think not.
void wxDiagram::RemoveAllShapes()
{
  m_shapeList->Clear();
}

void wxDiagram::DeleteAllShapes()
{
  auto node = m_shapeList->GetFirst();
  while (node)
  {
    wxShape *shape = (wxShape *)node->GetData();
    if (!shape->GetParent())
    {
      RemoveShape(shape);
      delete shape;
      node = m_shapeList->GetFirst();
    }
    else
      node = node->GetNext();
  }
}

void wxDiagram::ShowAll(bool show)
{
  auto current = m_shapeList->GetFirst();

  while (current)
  {
    wxShape *object = (wxShape *)current->GetData();
    object->Show(show);

    current = current->GetNext();
  }
}

void wxDiagram::DrawOutline(wxDC& dc, double x1, double y1, double x2, double y2)
{
  wxPen dottedPen(*wxBLACK, 1, wxPENSTYLE_DOT);
  dc.SetPen(dottedPen);
  dc.SetBrush((* wxTRANSPARENT_BRUSH));

  wxPoint points[5];

  points[0].x = (int) x1;
  points[0].y = (int) y1;

  points[1].x = (int) x2;
  points[1].y = (int) y1;

  points[2].x = (int) x2;
  points[2].y = (int) y2;

  points[3].x = (int) x1;
  points[3].y = (int) y2;

  points[4].x = (int) x1;
  points[4].y = (int) y1;
  dc.DrawLines(5, points);
}

// Make sure all text that should be centred, is centred.
void wxDiagram::RecentreAll(wxDC& dc)
{
  auto object_node = m_shapeList->GetFirst();
  while (object_node)
  {
    wxShape *obj = (wxShape *)object_node->GetData();
    obj->Recentre(dc);
    object_node = object_node->GetNext();
  }
}

// Input/output
void wxDiagram::SetCanvas(wxShapeCanvas *can)
{
  m_diagramCanvas = can;
}

// Find a shape by its id
wxShape* wxDiagram::FindShape(long id) const
{
    auto  node = GetShapeList()->GetFirst();
    while (node)
    {
        wxShape* shape = (wxShape*) node->GetData();
        if (shape->GetId() == id)
            return shape;
        node = node->GetNext();
    }
    return nullptr;
}


//// Crossings classes

wxLineCrossings::wxLineCrossings()
{
}

wxLineCrossings::~wxLineCrossings()
{
    ClearCrossings();
}

void wxLineCrossings::FindCrossings(wxDiagram& diagram)
{
    ClearCrossings();
    auto  node1 = diagram.GetShapeList()->GetFirst();
    while (node1)
    {
        wxShape* shape1 = (wxShape*) node1->GetData();
        if (shape1->IsKindOf(CLASSINFO(wxLineShape)))
        {
            wxLineShape* lineShape1 = (wxLineShape*) shape1;
            // Iterate through the segments
            const auto& pts1 = lineShape1->GetLineControlPoints();
            size_t i;
            for (i = 0; i < (pts1.size() - 1); i++)
            {
                const wxRealPoint& pt1_a = pts1[i];
                const wxRealPoint& pt1_b = pts1[i+1];

                // Now we iterate through the segments again

                auto  node2 = diagram.GetShapeList()->GetFirst();
                while (node2)
                {
                    wxShape* shape2 = (wxShape*) node2->GetData();

                    // Assume that the same line doesn't cross itself
                    if (shape2->IsKindOf(CLASSINFO(wxLineShape)) && (shape1 != shape2))
                    {
                        wxLineShape* lineShape2 = (wxLineShape*) shape2;
                        // Iterate through the segments
                        const auto& pts2 = lineShape2->GetLineControlPoints();
                        int j;
                        for (j = 0; j < (int) (pts2.size() - 1); j++)
                        {
                            const wxRealPoint& pt2_a = pts2[j];
                            const wxRealPoint& pt2_b = pts2[j+1];

                            // Now let's see if these two segments cross.
                            double ratio1, ratio2;
                            oglCheckLineIntersection(pt1_a.x, pt1_a.y, pt1_b.x, pt1_b.y,
                               pt2_a.x, pt2_a.y, pt2_b.x, pt2_b.y,
                             & ratio1, & ratio2);

                            if ((ratio1 < 1.0) && (ratio1 > -1.0))
                            {
                                // Intersection!
                                wxLineCrossing* crossing = new wxLineCrossing;
                                crossing->m_intersect.x = (pt1_a.x + (pt1_b.x - pt1_a.x)*ratio1);
                                crossing->m_intersect.y = (pt1_a.y + (pt1_b.y - pt1_a.y)*ratio1);

                                crossing->m_pt1 = pt1_a;
                                crossing->m_pt2 = pt1_b;
                                crossing->m_pt3 = pt2_a;
                                crossing->m_pt4 = pt2_b;

                                crossing->m_lineShape1 = lineShape1;
                                crossing->m_lineShape2 = lineShape2;

                                m_crossings.Append(crossing);
                            }
                        }
                    }
                    node2 = node2->GetNext();
                }
            }
        }

        node1 = node1->GetNext();
    }
}

void wxLineCrossings::DrawCrossings(wxDiagram& WXUNUSED(diagram), wxDC& dc)
{
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    long arcWidth = 8;

    auto  node = m_crossings.GetFirst();
    while (node)
    {
        wxLineCrossing* crossing = (wxLineCrossing*) node->GetData();
//        dc.DrawEllipse((long) (crossing->m_intersect.x - (arcWidth/2.0) + 0.5), (long) (crossing->m_intersect.y - (arcWidth/2.0) + 0.5),
//           arcWidth, arcWidth);


        // Let's do some geometry to find the points on either end of the arc.
/*

(x1, y1)
    |\
    | \
    |  \
    |   \
    |    \
    |    |\ c    c1
    |  a | \
         |  \
    |     -  x <-- centre of arc
 a1 |     b  |\
    |        | \ c2
    |     a2 |  \
    |         -  \
    |         b2  \
    |              \
    |_______________\ (x2, y2)
          b1

*/

        double a1 = wxMax(crossing->m_pt1.y, crossing->m_pt2.y) - wxMin(crossing->m_pt1.y, crossing->m_pt2.y) ;
        double b1 = wxMax(crossing->m_pt1.x, crossing->m_pt2.x) - wxMin(crossing->m_pt1.x, crossing->m_pt2.x) ;
        double c1 = sqrt( (a1*a1) + (b1*b1) );

        double c = arcWidth / 2.0;
        double a = c * a1/c1 ;
        double b = c * b1/c1 ;

        // I'm not sure this is right, since we don't know which direction we should be going in - need
        // to know which way the line slopes and choose the sign appropriately.
        double arcX1 = crossing->m_intersect.x - b;
        double arcY1 = crossing->m_intersect.y - a;

        double arcX2 = crossing->m_intersect.x + b;
        double arcY2 = crossing->m_intersect.y + a;

        dc.SetPen(*wxBLACK_PEN);
        dc.DrawArc( (long) arcX1, (long) arcY1, (long) arcX2, (long) arcY2,
            (long) crossing->m_intersect.x, (long) crossing->m_intersect.y);

        dc.SetPen(*wxWHITE_PEN);
        dc.DrawLine( (long) arcX1, (long) arcY1, (long) arcX2, (long) arcY2 );

        node = node->GetNext();
    }
}

void wxLineCrossings::ClearCrossings()
{
    auto  node = m_crossings.GetFirst();
    while (node)
    {
        wxLineCrossing* crossing = (wxLineCrossing*) node->GetData();
        delete crossing;
        node = node->GetNext();
    }
    m_crossings.Clear();
}

