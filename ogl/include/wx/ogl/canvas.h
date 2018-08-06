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

#include "wx/overlay.h"

// Drag states
#define NoDragging             0
#define StartDraggingLeft      1
#define ContinueDraggingLeft   2
#define StartDraggingRight     3
#define ContinueDraggingRight  4

WXDLLIMPEXP_OGL extern const wxChar* wxShapeCanvasNameStr;

// When drag_count reaches 0, process drag message

class WXDLLIMPEXP_OGL wxDiagram;

class WXDLLIMPEXP_OGL wxShapeCanvas: public wxScrolledWindow
{
 DECLARE_DYNAMIC_CLASS(wxShapeCanvas)
 public:
  wxShapeCanvas(wxWindow *parent = NULL, wxWindowID id = wxID_ANY,
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

  // Find object for mouse click, of given wxClassInfo (NULL for any type).
  // If notImage is non-NULL, don't find an object that is equal to or a descendant of notImage
  virtual wxShape *FindShape(double x, double y, int *attachment, wxClassInfo *info = NULL, wxShape *notImage = NULL);
  wxShape *FindFirstSensitiveShape(double x, double y, int *new_attachment, int op);
  wxShape *FindFirstSensitiveShape1(wxShape *image, int op);

  // Redirect to wxDiagram object
  virtual void AddShape(wxShape *object, wxShape *addAfter = NULL);
  virtual void InsertShape(wxShape *object);
  virtual void RemoveShape(wxShape *object);
  virtual bool GetQuickEditMode();
  virtual void Redraw(wxDC& dc);
  void Snap(double *x, double *y);

  // Events
  void OnPaint(wxPaintEvent& event);
  void OnMouseEvent(wxMouseEvent& event);

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
// on it as usual. To erase everything previously drawn over the canvas, do not
// call GetDC() and call Reset() instead.
class wxShapeCanvasOverlay
{
public:
  explicit wxShapeCanvasOverlay(wxShapeCanvas* canvas)
    : m_dc(canvas),
      m_overlay(canvas->m_overlay),
      m_overlayReset(m_overlay),
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

  void Reset()
  {
    // We can't call Reset() from here as it can't be done for as long as
    // m_dcOverlay exists, so just set a flag and actually do it later, in
    // m_overlayReset dtor, after m_dcOverlay is destroyed.
    m_overlayReset.Do();
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

  // Another helper class used to conditionally call wxOverlay::Reset().
  class OverlayReset
  {
  public:
    explicit OverlayReset(wxOverlay& overlay)
      : m_overlay(overlay),
        m_reset(false)
    {
    }

    void Do()
    {
      m_reset = true;
    }

    ~OverlayReset()
    {
      if (m_reset)
        m_overlay.Reset();
    }

  private:
    wxOverlay& m_overlay;
    bool m_reset;
  };

  OverlayReset m_overlayReset;

  // Note that order of declarations here is important: m_dcOverlay must come
  // after m_overlayReset, so that it is destroyed first.
  wxDCOverlay m_dcOverlay;

  wxDECLARE_NO_COPY_CLASS(wxShapeCanvasOverlay);
};

#endif
 // _OGL_CANVAS_H_
