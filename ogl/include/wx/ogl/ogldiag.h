/////////////////////////////////////////////////////////////////////////////
// Name:        ogldiag.h
// Purpose:     OGL - wxDiagram class
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_OGLDIAG_H_
#define _OGL_OGLDIAG_H_

#include "wx/ogl/basic.h"

#define oglHAVE_XY_GRID

class WXDLLIMPEXP_OGL wxDiagram: public wxObject
{
 DECLARE_DYNAMIC_CLASS(wxDiagram)

public:

  wxDiagram();
  virtual ~wxDiagram();

  void SetCanvas(wxShapeCanvas *can);

  inline wxShapeCanvas *GetCanvas() const { return m_diagramCanvas; }

  virtual void Redraw(wxDC& dc);
  virtual void Clear(wxDC& dc);
  virtual void DrawOutline(wxDC& dc, double x1, double y1, double x2, double y2);

  // Add object to end of object list (if addAfter is nullptr)
  // or just after addAfter.
  virtual void AddShape(wxShape *object, wxShape *addAfter = nullptr);

  // Add object to front of object list
  virtual void InsertShape(wxShape *object);

  void SetSnapToGrid(bool snap);
  void SetGridSpacing(double spacing) { SetGridSpacing(spacing, spacing); }
  void SetGridSpacing(double x, double y);
  inline double GetGridSpacing() const { return m_gridSpacing; }
  void GetGridSpacing(double *x, double *y) const;
  inline bool GetSnapToGrid() const { return m_snapToGrid; }
  void Snap(double *x, double *y);

  virtual void RemoveShape(wxShape *object);
  virtual void RemoveAllShapes();
  virtual void DeleteAllShapes();
  virtual void ShowAll(bool show);

  // Find a shape by its id
  wxShape* FindShape(long id) const;

  inline void SetMouseTolerance(int tol) { m_mouseTolerance = tol; }
  inline int GetMouseTolerance() const { return m_mouseTolerance; }
  inline wxList *GetShapeList() const { return m_shapeList; }
  inline int GetCount() const { return m_shapeList->GetCount(); }

  // Make sure all text that should be centred, is centred.
  void RecentreAll(wxDC& dc);

protected:
  wxShapeCanvas*        m_diagramCanvas;
  bool                  m_snapToGrid;
  double                m_gridSpacing;
  double                m_gridSpacingAspect;
  int                   m_mouseTolerance;
  wxList*               m_shapeList;
};

class WXDLLIMPEXP_OGL wxLineCrossing: public wxObject
{
public:
    wxLineCrossing() { m_lineShape1 = nullptr; m_lineShape2 = nullptr; }
    wxRealPoint     m_pt1; // First line
    wxRealPoint     m_pt2;
    wxRealPoint     m_pt3; // Second line
    wxRealPoint     m_pt4;
    wxRealPoint     m_intersect;
    wxLineShape*    m_lineShape1;
    wxLineShape*    m_lineShape2;
};

class WXDLLIMPEXP_OGL wxLineCrossings: public wxObject
{
public:
    wxLineCrossings();
    ~wxLineCrossings();

    void FindCrossings(wxDiagram& diagram);
    void DrawCrossings(wxDiagram& diagram, wxDC& dc);
    void ClearCrossings();

public:
    wxList  m_crossings;
};

#endif
 // _OGL_OGLDIAG_H_
