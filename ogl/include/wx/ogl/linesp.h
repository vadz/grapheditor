/////////////////////////////////////////////////////////////////////////////
// Name:        linesp.h
// Purpose:     Lines private header file
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_LINESP_H_
#define _OGL_LINESP_H_


class WXDLLIMPEXP_OGL wxLineShape;
class WXDLLIMPEXP_OGL wxLineControlPoint: public wxControlPoint
{
  DECLARE_DYNAMIC_CLASS(wxLineControlPoint)
  friend class WXDLLIMPEXP_OGL wxLineShape;
 public:

  wxLineControlPoint(wxShapeCanvas *the_canvas = nullptr, wxShape *object = nullptr, double size = 0.0,
     double x = 0.0, double y = 0.0, int the_type = 0);
  ~wxLineControlPoint();

  void OnDraw(wxDC& dc) override;
  void OnDragLeft(bool draw, double x, double y, int keys=0, int attachment = 0) override;
  void OnBeginDragLeft(double x, double y, int keys=0, int attachment = 0) override;
  void OnEndDragLeft(double x, double y, int keys=0, int attachment = 0) override;

  // Obsolete (left-dragging now moves attachment point to new relative position OR new
  // attachment id)
#if 0
  void OnDragRight(bool draw, double x, double y, int keys=0, int attachment = 0);
  void OnBeginDragRight(double x, double y, int keys=0, int attachment = 0);
  void OnEndDragRight(double x, double y, int keys=0, int attachment = 0);
#endif

public:

  int           m_type;
  wxRealPoint   m_point;  // Line point
  wxRealPoint   m_originalPos;

};

/*
 * Temporary arc label object
 */

class WXDLLIMPEXP_OGL wxLabelShape: public wxRectangleShape
{
  DECLARE_DYNAMIC_CLASS(wxLabelShape)

 public:
  wxLabelShape(wxLineShape *parent = nullptr, wxShapeRegion *region = nullptr, double w = 0.0, double h = 0.0);
  ~wxLabelShape();

  void OnDraw(wxDC& dc) override;
  void OnDrawContents(wxDC& dc) override;
  void OnLeftClick(double x, double y, int keys = 0, int attachment = 0) override;
  void OnRightClick(double x, double y, int keys = 0, int attachment = 0) override;
  void OnDragLeft(bool draw, double x, double y, int keys=0, int attachment = 0) override;
  void OnBeginDragLeft(double x, double y, int keys=0, int attachment = 0) override;
  void OnEndDragLeft(double x, double y, int keys=0, int attachment = 0) override;
  bool OnMovePre(wxReadOnlyDC& dc, double x, double y, double old_x, double old_y, bool display = true) override;

public:
  wxLineShape*      m_lineShape;
  wxShapeRegion*    m_shapeRegion;

};

/*
 * Get the point on the given line (x1, y1) (x2, y2)
 * distance 'length' along from the end,
 * returned values in x and y
 */

void GetPointOnLine(double x1, double y1, double x2, double y2,
                    double length, double *x, double *y);

#endif
    // _OGL_LINESP_H_

