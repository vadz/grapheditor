/////////////////////////////////////////////////////////////////////////////
// Name:        canvas.h
// Purpose:     wxShapeCanvas
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_CANVAS_H_
#define _OGL_CANVAS_H_

#include "wx/ogl/defs.h"

#include "wx/dcclient.h"
#include "wx/overlay.h"
#include "wx/scrolwin.h"

// Drag states
#define NoDragging             0
#define StartDraggingLeft      1
#define ContinueDraggingLeft   2
#define StartDraggingRight     3
#define ContinueDraggingRight  4

WXDLLIMPEXP_OGL extern const wxChar* wxShapeCanvasNameStr;

// When drag_count reaches 0, process drag message

class WXDLLIMPEXP_OGL wxDiagram;
class WXDLLIMPEXP_OGL wxShape;

class WXDLLIMPEXP_OGL wxShapeCanvas: public wxScrolledWindow
{
 DECLARE_DYNAMIC_CLASS(wxShapeCanvas)
 public:
  wxShapeCanvas(wxWindow *parent = nullptr, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxBORDER | wxRETAINED,
                const wxString& name = wxShapeCanvasNameStr);
  ~wxShapeCanvas();

  inline void SetDiagram(wxDiagram *diag) { m_shapeDiagram = diag; }
  inline wxDiagram *GetDiagram() const { return m_shapeDiagram; }

  virtual void OnLeftClick(double x, double y, int keys = 0);
  virtual void OnRightClick(double x, double y, int keys = 0);

  virtual void OnDragLeft(bool draw, double x, double y, int keys=0); // Erase if draw false
  virtual void OnBeginDragLeft(double x, double y, int keys=0);
  virtual void OnEndDragLeft(double x, double y, int keys=0);

  virtual void OnDragRight(bool draw, double x, double y, int keys=0); // Erase if draw false
  virtual void OnBeginDragRight(double x, double y, int keys=0);
  virtual void OnEndDragRight(double x, double y, int keys=0);

  // Find object for mouse click, of given wxClassInfo (nullptr for any type).
  // If notImage is non-nullptr, don't find an object that is equal to or a descendant of notImage
  virtual wxShape *FindShape(double x, double y, int *attachment, wxClassInfo *info = nullptr, wxShape *notImage = nullptr);
  wxShape *FindFirstSensitiveShape(double x, double y, int *new_attachment, int op);
  wxShape *FindFirstSensitiveShape1(wxShape *image, int op);

  // Redirect to wxDiagram object
  virtual void AddShape(wxShape *object, wxShape *addAfter = nullptr);
  virtual void InsertShape(wxShape *object);
  virtual void RemoveShape(wxShape *object);
  virtual void Redraw(wxDC& dc);
  void Snap(double *x, double *y);

  // Clear any temporarily drawn hints.
  void ClearHints() { m_overlay.Reset(); }

  // Release mouse and clear hints.
  void EndDrag();

  // Events
  void OnPaint(wxPaintEvent& event);
  void OnMouseEvent(wxMouseEvent& event);
  void OnCaptureLost(wxMouseCaptureLostEvent& event);

 protected:
  wxDiagram*        m_shapeDiagram;
  int               m_dragState;
  double             m_oldDragX, m_oldDragY;     // Previous drag coordinates
  double             m_firstDragX, m_firstDragY; // INITIAL drag coordinates
  bool              m_checkTolerance;           // Whether to check drag tolerance
  wxShape*          m_draggedShape;
  int               m_draggedAttachment;

 private:
  wxOverlay         m_overlay;

  // Give friendship to wxShapeCanvasOverlay to allow it accessing m_overlay.
  friend class wxShapeCanvasOverlay;

DECLARE_EVENT_TABLE()
};

// Helper class for temporary drawing over the shape canvas.
//
// To draw something over the canvas, call GetDC() to retrieve the DC and draw
// on it as usual. Just creating this object erases everything previously drawn
// on the overlay.
class wxShapeCanvasOverlay
{
public:
  explicit wxShapeCanvasOverlay(wxShapeCanvas* canvas)
    : m_dc(canvas),
      m_overlay(canvas->m_overlay),
      m_dcOverlay(m_overlay, &m_dc)
  {
    // Start by clearing the previously drawn contents in any case.
    m_dcOverlay.Clear();
  }

  // When drawing, this method can be used to retrieve the DC to use.
  wxDC& GetDC()
  {
    return m_dc;
  }

private:
  // This class exists only in order to call PrepareDC() in its ctor and thus
  // allow fully initializing m_dc in the ctor initializer list.
  class PreparedDC : public wxClientDC
  {
  public:
    explicit PreparedDC(wxShapeCanvas* canvas)
      : wxClientDC(canvas)
    {
      canvas->PrepareDC(*this);
    }
  };

  PreparedDC m_dc;

  wxOverlay& m_overlay;

  wxDCOverlay m_dcOverlay;

  wxDECLARE_NO_COPY_CLASS(wxShapeCanvasOverlay);
};

#endif
 // _OGL_CANVAS_H_
