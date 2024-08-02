/////////////////////////////////////////////////////////////////////////////
// Name:        basic.h
// Purpose:     Basic OGL classes and definitions
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_BASIC_H_
#define _OGL_BASIC_H_

#include "wx/ogl/defs.h"

#include "wx/clntdata.h"
#include "wx/dc.h"

#define OGL_VERSION     2.0

#ifndef DEFAULT_MOUSE_TOLERANCE
#define DEFAULT_MOUSE_TOLERANCE 3
#endif

// Key identifiers
#define KEY_SHIFT 1
#define KEY_CTRL  2

// Arrow styles

#define ARROW_NONE         0
#define ARROW_END          1
#define ARROW_BOTH         2
#define ARROW_MIDDLE       3
#define ARROW_START        4

// Control point types
// Rectangle and most other shapes
#define CONTROL_POINT_VERTICAL   1
#define CONTROL_POINT_HORIZONTAL 2
#define CONTROL_POINT_DIAGONAL   3

// Line
#define CONTROL_POINT_ENDPOINT_TO 4
#define CONTROL_POINT_ENDPOINT_FROM 5
#define CONTROL_POINT_LINE       6

// Types of formatting: can be combined in a bit list
#define FORMAT_NONE           0
                                // Left justification
#define FORMAT_CENTRE_HORIZ   1
                                // Centre horizontally
#define FORMAT_CENTRE_VERT    2
                                // Centre vertically
#define FORMAT_SIZE_TO_CONTENTS 4
                                // Resize shape to contents

// Shadow mode
#define SHADOW_NONE           0
#define SHADOW_LEFT           1
#define SHADOW_RIGHT          2

/*
 * Declare types
 *
 */

#define SHAPE_BASIC           wxTYPE_USER + 1
#define SHAPE_RECTANGLE       wxTYPE_USER + 2
#define SHAPE_ELLIPSE         wxTYPE_USER + 3
#define SHAPE_POLYGON         wxTYPE_USER + 4
#define SHAPE_CIRCLE          wxTYPE_USER + 5
#define SHAPE_LINE            wxTYPE_USER + 6
#define SHAPE_DIVIDED_RECTANGLE wxTYPE_USER + 8
#define SHAPE_COMPOSITE       wxTYPE_USER + 9
#define SHAPE_CONTROL_POINT   wxTYPE_USER + 10
#define SHAPE_DRAWN           wxTYPE_USER + 11
#define SHAPE_DIVISION        wxTYPE_USER + 12
#define SHAPE_LABEL_OBJECT    wxTYPE_USER + 13
#define SHAPE_BITMAP          wxTYPE_USER + 14
#define SHAPE_DIVIDED_OBJECT_CONTROL_POINT   wxTYPE_USER + 15

#define OBJECT_REGION         wxTYPE_USER + 20

#define OP_CLICK_LEFT  1
#define OP_CLICK_RIGHT 2
#define OP_DRAG_LEFT 4
#define OP_DRAG_RIGHT 8

#define OP_ALL (OP_CLICK_LEFT | OP_CLICK_RIGHT | OP_DRAG_LEFT | OP_DRAG_RIGHT)

// Attachment modes
#define ATTACHMENT_MODE_NONE        0
#define ATTACHMENT_MODE_EDGE        1
#define ATTACHMENT_MODE_BRANCHING   2

// Sub-modes for branching attachment mode
#define BRANCHING_ATTACHMENT_NORMAL 1
#define BRANCHING_ATTACHMENT_BLOB   2

class wxShapeTextLine;
class wxShapeCanvas;
class wxLineShape;
class wxControlPoint;
class wxShapeRegion;
class wxShape;

// Round up
#define WXROUND(x) ( (long) (x + 0.5) )


class WXDLLIMPEXP_OGL wxShapeEvtHandler: public wxObject, public wxClientDataContainer
{
 DECLARE_DYNAMIC_CLASS(wxShapeEvtHandler)

 public:
  wxShapeEvtHandler(wxShapeEvtHandler *prev = nullptr, wxShape *shape = nullptr);
  virtual ~wxShapeEvtHandler();

  inline void SetShape(wxShape *sh) { m_handlerShape = sh; }
  inline wxShape *GetShape() const { return m_handlerShape; }

  inline void SetPreviousHandler(wxShapeEvtHandler* handler) { m_previousHandler = handler; }
  inline wxShapeEvtHandler* GetPreviousHandler() const { return m_previousHandler; }

  // This is called when the _shape_ is deleted.
  virtual void OnDelete();
  virtual void OnDraw(wxDC& dc);
  virtual void OnDrawContents(wxDC& dc);
  virtual void OnDrawBranches(wxDC& dc, bool erase = false);
  virtual void OnMoveLinks(wxReadOnlyDC& dc);
  virtual void OnErase(wxReadOnlyDC& dc);
  virtual void OnEraseContents(wxReadOnlyDC& dc);
  virtual void OnEraseBranches(wxReadOnlyDC& dc);
  virtual void OnHighlight(wxDC& dc);
  virtual void OnLeftClick(double x, double y, int keys = 0, int attachment = 0);
  virtual void OnLeftDoubleClick(double x, double y, int keys = 0, int attachment = 0);
  virtual void OnRightClick(double x, double y, int keys = 0, int attachment = 0);
  virtual void OnSize(double x, double y);
  virtual bool OnMovePre(wxReadOnlyDC& dc, double x, double y, double old_x, double old_y, bool display = true);
  virtual void OnMovePost(wxReadOnlyDC& dc, double x, double y, double old_x, double old_y, bool display = true);

  virtual void OnDragLeft(bool draw, double x, double y, int keys=0, int attachment = 0); // Erase if draw false
  virtual void OnBeginDragLeft(double x, double y, int keys=0, int attachment = 0);
  virtual void OnEndDragLeft(double x, double y, int keys=0, int attachment = 0);
  virtual void OnDragRight(bool draw, double x, double y, int keys=0, int attachment = 0); // Erase if draw false
  virtual void OnBeginDragRight(double x, double y, int keys=0, int attachment = 0);
  virtual void OnEndDragRight(double x, double y, int keys=0, int attachment = 0);
  virtual void OnDrawOutline(wxDC& dc, double x, double y, double w, double h);
  virtual void OnDrawControlPoints(wxDC& dc);
  virtual void OnEraseControlPoints(wxReadOnlyDC& dc);
  virtual void OnMoveLink(wxReadOnlyDC& dc, bool moveControlPoints = true);

  // Control points ('handles') redirect control to the actual shape, to make it easier
  // to override sizing behaviour.
  virtual void OnSizingDragLeft(wxControlPoint* pt, bool draw, double x, double y, int keys=0, int attachment = 0); // Erase if draw false
  virtual void OnSizingBeginDragLeft(wxControlPoint* pt, double x, double y, int keys=0, int attachment = 0);
  virtual void OnSizingEndDragLeft(wxControlPoint* pt, double x, double y, int keys=0, int attachment = 0);

  virtual void OnBeginSize(double WXUNUSED(w), double WXUNUSED(h)) { }
  virtual void OnEndSize(double WXUNUSED(w), double WXUNUSED(h)) { }

  // Can override this to prevent or intercept line reordering.
  virtual void OnChangeAttachment(int attachment, wxLineShape* line, wxList& ordering);

  // Creates a copy of this event handler.
  wxShapeEvtHandler *CreateNewCopy();

  // Does the copy - override for new event handlers which might store
  // app-specific data.
  virtual void CopyData(wxShapeEvtHandler& WXUNUSED(copy)) {};

 private:
  wxShapeEvtHandler*    m_previousHandler;
  wxShape*              m_handlerShape;
};

class WXDLLIMPEXP_OGL wxShape: public wxShapeEvtHandler
{
 DECLARE_ABSTRACT_CLASS(wxShape)

 public:

  wxShape(wxShapeCanvas *can = nullptr);
  virtual ~wxShape();
  virtual void GetBoundingBoxMax(double *width, double *height);
  virtual void GetBoundingBoxMin(double *width, double *height) = 0;
  virtual bool GetPerimeterPoint(double x1, double y1,
                                 double x2, double y2,
                                 double *x3, double *y3);
  inline wxShapeCanvas *GetCanvas() { return m_canvas; }
  void SetCanvas(wxShapeCanvas *the_canvas);
  virtual void AddToCanvas(wxShapeCanvas *the_canvas, wxShape *addAfter = nullptr);
  virtual void InsertInCanvas(wxShapeCanvas *the_canvas);

  virtual void RemoveFromCanvas(wxShapeCanvas *the_canvas);
  inline double GetX() const { return m_xpos; }
  inline double GetY() const { return m_ypos; }
  inline void SetX(double x) { m_xpos = x; }
  inline void SetY(double y) { m_ypos = y; }

  inline wxShape *GetParent() const { return m_parent; }
  inline void SetParent(wxShape *p) { m_parent = p; }
  wxShape *GetTopAncestor();
  inline wxList& GetChildren() { return m_children; }

  // Redraw the shape during the next paint cycle.
  void Redraw();

  void OnDraw(wxDC& dc) override;
  void OnDrawContents(wxDC& dc) override;
  void OnMoveLinks(wxReadOnlyDC& dc) override;
  virtual void Unlink() { };
  void SetDrawHandles(bool drawH);
  inline bool GetDrawHandles() { return m_drawHandles; }
  void OnErase(wxReadOnlyDC& dc) override;
  void OnEraseContents(wxReadOnlyDC& dc) override;
  void OnHighlight(wxDC& dc) override;
  void OnLeftClick(double x, double y, int keys = 0, int attachment = 0) override;
  void OnLeftDoubleClick(double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys) = 0, int WXUNUSED(attachment) = 0) override {}
  void OnRightClick(double x, double y, int keys = 0, int attachment = 0) override;
  void OnSize(double x, double y) override;
  bool OnMovePre(wxReadOnlyDC& dc, double x, double y, double old_x, double old_y, bool display = true) override;
  void OnMovePost(wxReadOnlyDC& dc, double x, double y, double old_x, double old_y, bool display = true) override;

  void OnDragLeft(bool draw, double x, double y, int keys=0, int attachment = 0) override; // Erase if draw false
  void OnBeginDragLeft(double x, double y, int keys=0, int attachment = 0) override;
  void OnEndDragLeft(double x, double y, int keys=0, int attachment = 0) override;
  void OnDragRight(bool draw, double x, double y, int keys=0, int attachment = 0) override; // Erase if draw false
  void OnBeginDragRight(double x, double y, int keys=0, int attachment = 0) override;
  void OnEndDragRight(double x, double y, int keys=0, int attachment = 0) override;
  void OnDrawOutline(wxDC& dc, double x, double y, double w, double h) override;
  void OnDrawControlPoints(wxDC& dc) override;
  void OnEraseControlPoints(wxReadOnlyDC& dc) override;

  void OnBeginSize(double WXUNUSED(w), double WXUNUSED(h)) override { }
  void OnEndSize(double WXUNUSED(w), double WXUNUSED(h)) override { }

  // Control points ('handles') redirect control to the actual shape, to make it easier
  // to override sizing behaviour.
  void OnSizingDragLeft(wxControlPoint* pt, bool draw, double x, double y, int keys=0, int attachment = 0) override; // Erase if draw false
  void OnSizingBeginDragLeft(wxControlPoint* pt, double x, double y, int keys=0, int attachment = 0) override;
  void OnSizingEndDragLeft(wxControlPoint* pt, double x, double y, int keys=0, int attachment = 0) override;

  virtual void MakeControlPoints();
  virtual void DeleteControlPoints(wxReadOnlyDC *dc = nullptr);
  virtual void ResetControlPoints();

  inline wxShapeEvtHandler *GetEventHandler() { return m_eventHandler; }
  inline void SetEventHandler(wxShapeEvtHandler *handler) { m_eventHandler = handler; }

  // Mandatory control points, e.g. the divided line moving handles
  // should appear even if a child of the 'selected' image
  virtual void MakeMandatoryControlPoints();
  virtual void ResetMandatoryControlPoints();

  inline virtual bool Recompute() { return true; };
  // Calculate size recursively, if size changes. Size might depend on children.
  inline virtual void CalculateSize() { };
  virtual void Select(bool select = true, wxReadOnlyDC* dc = nullptr);
  virtual void SetHighlight(bool hi = true, bool recurse = false);
  inline virtual bool IsHighlighted() const { return m_highlighted; };
  virtual bool Selected() const;
  virtual bool AncestorSelected() const;
  void SetSensitivityFilter(int sens = OP_ALL, bool recursive = false);
  int GetSensitivityFilter() const { return m_sensitivity; }
  void SetDraggable(bool drag, bool recursive = false);
  inline  void SetFixedSize(bool x, bool y) { m_fixedWidth = x; m_fixedHeight = y; };
  inline  void GetFixedSize(bool *x, bool *y) const { *x = m_fixedWidth; *y = m_fixedHeight; };
  inline  void SetFixedWidth(bool fixed = true) { m_fixedWidth = fixed; }
  inline  void SetFixedHeight(bool fixed = true) { m_fixedHeight = fixed; }
  inline  bool GetFixedWidth() const { return m_fixedWidth; }
  inline  bool GetFixedHeight() const { return m_fixedHeight; }
  inline  void SetSpaceAttachments(bool sp) { m_spaceAttachments = sp; };
  inline  bool GetSpaceAttachments() const { return m_spaceAttachments; };
  void SetShadowMode(int mode, bool redraw = false);
  inline int GetShadowMode() const { return m_shadowMode; }
  virtual bool HitTest(double x, double y, int *attachment, double *distance);
  inline void SetCentreResize(bool cr) { m_centreResize = cr; }
  inline bool GetCentreResize() const { return m_centreResize; }
  inline void SetMaintainAspectRatio(bool ar) { m_maintainAspectRatio = ar; }
  inline bool GetMaintainAspectRatio() const { return m_maintainAspectRatio; }
  inline wxList& GetLines() const { return (wxList&) m_lines; }
  inline void SetDisableLabel(bool flag) { m_disableLabel = flag; }
  inline bool GetDisableLabel() const { return m_disableLabel; }
  inline void SetAttachmentMode(int mode) { m_attachmentMode = mode; }
  inline int GetAttachmentMode() const { return m_attachmentMode; }
  inline void SetId(long i) { m_id = i; }
  inline long GetId() const { return m_id; }

  void SetPen(const wxPen *pen);
  void SetBrush(const wxBrush *brush);

  virtual void Show(bool show);
  virtual bool IsShown() const { return m_visible; }
  virtual void Move(wxReadOnlyDC& dc, double x1, double y1, bool display = true);
  virtual void Erase(wxReadOnlyDC& dc);
  virtual void EraseContents(wxReadOnlyDC& dc);
  virtual void Draw(wxDC& dc);
  virtual void MoveLinks(wxReadOnlyDC& dc);
  virtual void DrawContents(wxDC& dc);  // E.g. for drawing text label
  virtual void SetSize(double x, double y, bool recursive = true);
  virtual void SetAttachmentSize(double x, double y);
  void Attach(wxShapeCanvas *can);
  void Detach();

  inline virtual bool Constrain() { return false; } ;

  void AddLine(wxLineShape *line, wxShape *other,
               int attachFrom = 0, int attachTo = 0,
               // The line ordering
               int positionFrom = -1, int positionTo = -1);

  // Return the zero-based position in m_lines of line.
  int GetLinePosition(wxLineShape* line);

  void AddText(const wxString& string);

  inline wxPen *GetPen() const { return wx_const_cast(wxPen*, m_pen); }
  inline wxBrush *GetBrush() const { return wx_const_cast(wxBrush*, m_brush); }

  /*
   * Region-specific functions (defaults to the default region
   * for simple objects
   */

  // Set the default, single region size to be consistent
  // with the object size
  void SetDefaultRegionSize();
  virtual void FormatText(wxReadOnlyDC& dc, const wxString& s, int regionId = 0);
  virtual void SetFormatMode(int mode, int regionId = 0);
  virtual int GetFormatMode(int regionId = 0) const;
  virtual void SetFont(wxFont *font, int regionId = 0);
  virtual wxFont *GetFont(int regionId = 0) const;
  virtual void SetTextColour(const wxString& colour, int regionId = 0);
  virtual wxString GetTextColour(int regionId = 0) const;
  virtual inline int GetNumberOfTextRegions() const { return m_regions.GetCount(); }
  virtual void SetRegionName(const wxString& name, int regionId = 0);

  // Get the name representing the region for this image alone.
  // I.e. this image's region ids go from 0 to N-1.
  // But the names might be "0.2.0", "0.2.1" etc. depending on position in composite.
  // So the last digit represents the region Id, the others represent positions
  // in composites.
  virtual wxString GetRegionName(int regionId);

  // Gets the region corresponding to the name, or -1 if not found.
  virtual int GetRegionId(const wxString& name);

  // Construct names for regions, unique even for children of a composite.
  virtual void NameRegions(const wxString& parentName = wxEmptyString);

  // Get list of regions
  inline wxList& GetRegions() const { return (wxList&) m_regions; }

  virtual void AddRegion(wxShapeRegion *region);

  virtual void ClearRegions();

  // Assign new ids to this image and children (if composite)
  void AssignNewIds();

  // Returns actual image (same as 'this' if non-composite) and region id
  // for given region name.
  virtual wxShape *FindRegion(const wxString& regionName, int *regionId);

  // Finds all region names for this image (composite or simple).
  // Supply empty string list.
  virtual void FindRegionNames(wxStringList& list);

  virtual void ClearText(int regionId = 0);
  void RemoveLine(wxLineShape *line);

  // Attachment code
  virtual bool GetAttachmentPosition(int attachment, double *x, double *y,
                                     int nth = 0, int no_arcs = 1, wxLineShape *line = nullptr);
  virtual int GetNumberOfAttachments() const;
  virtual bool AttachmentIsValid(int attachment) const;
  virtual wxList& GetAttachments() { return m_attachmentPoints; }

  // Only get the attachment position at the _edge_ of the shape, ignoring
  // branching mode. This is used e.g. to indicate the edge of interest, not the point
  // on the attachment branch.
  virtual bool GetAttachmentPositionEdge(int attachment, double *x, double *y,
                                     int nth = 0, int no_arcs = 1, wxLineShape *line = nullptr);

  // Assuming the attachment lies along a vertical or horizontal line,
  // calculate the position on that point.
  virtual wxRealPoint CalcSimpleAttachment(const wxRealPoint& pt1, const wxRealPoint& pt2,
    int nth, int noArcs, wxLineShape* line);

  // Returns true if pt1 <= pt2 in the sense that one point comes before another on an
  // edge of the shape.
  // attachmentPoint is the attachment point (= side) in question.
  virtual bool AttachmentSortTest(int attachmentPoint, const wxRealPoint& pt1, const wxRealPoint& pt2);

  virtual void EraseLinks(wxReadOnlyDC& dc, int attachment = -1, bool recurse = false);
  virtual void DrawLinks(wxDC& dc, int attachment = -1, bool recurse = false);

  virtual bool MoveLineToNewAttachment(wxReadOnlyDC& dc, wxLineShape *to_move,
                                       double x, double y);

  // Reorders the lines coming into the node image at this attachment
  // position, in the order in which they appear in linesToSort.
  virtual void SortLines(int attachment, wxList& linesToSort);

  // Apply an attachment ordering change
  void ApplyAttachmentOrdering(wxList& ordering);

  // Can override this to prevent or intercept line reordering.
  void OnChangeAttachment(int attachment, wxLineShape* line, wxList& ordering) override;

  //// New banching attachment code, 24/9/98

  //
  //             |________|
  //                 | <- root
  //                 | <- neck
  // shoulder1 ->---------<- shoulder2
  //             | | | | |<- stem
  //                      <- branching attachment point N-1

  // This function gets the root point at the given attachment.
  virtual wxRealPoint GetBranchingAttachmentRoot(int attachment);

  // This function gets information about where branching connections go (calls GetBranchingAttachmentRoot)
  virtual bool GetBranchingAttachmentInfo(int attachment, wxRealPoint& root, wxRealPoint& neck,
    wxRealPoint& shoulder1, wxRealPoint& shoulder2);

  // n is the number of the adjoining line, from 0 to N-1 where N is the number of lines
  // at this attachment point.
  // attachmentPoint is where the arc meets the stem, and stemPoint is where the stem meets the
  // shoulder.
  virtual bool GetBranchingAttachmentPoint(int attachment, int n, wxRealPoint& attachmentPoint,
    wxRealPoint& stemPoint);

  // Get the number of lines at this attachment position.
  virtual int GetAttachmentLineCount(int attachment) const;

  // Draw the branches (not the actual arcs though)
  virtual void OnDrawBranches(wxDC& dc, int attachment, bool erase = false);
  void OnDrawBranches(wxDC& dc, bool erase = false) override;

  virtual void OnEraseBranches(wxReadOnlyDC& dc, int attachment);
  void OnEraseBranches(wxReadOnlyDC& dc) override;

  // Branching attachment settings
  inline void SetBranchNeckLength(int len) { m_branchNeckLength = len; }
  inline int GetBranchNeckLength() const { return m_branchNeckLength; }

  inline void SetBranchStemLength(int len) { m_branchStemLength = len; }
  inline int GetBranchStemLength() const { return m_branchStemLength; }

  inline void SetBranchSpacing(int len) { m_branchSpacing = len; }
  inline int GetBranchSpacing() const { return m_branchSpacing; }

  // Further detail on branching style, e.g. blobs on interconnections
  inline void SetBranchStyle(long style) { m_branchStyle = style; }
  inline long GetBranchStyle() const { return m_branchStyle; }

  // Rotate the standard attachment point from physical (0 is always North)
  // to logical (0 -> 1 if rotated by 90 degrees)
  virtual int PhysicalToLogicalAttachment(int physicalAttachment) const;

  // Rotate the standard attachment point from logical
  // to physical (0 is always North)
  virtual int LogicalToPhysicalAttachment(int logicalAttachment) const;

  // This is really to distinguish between lines and other images.
  // For lines, want to pass drag to canvas, since lines tend to prevent
  // dragging on a canvas (they get in the way.)
  virtual bool Draggable() const { return true; }

  // Returns true if image is a descendant of this image
  bool HasDescendant(wxShape *image);

  // Creates a copy of this shape.
  wxShape *CreateNewCopy(bool resetMapping = true, bool recompute = true);

  // Does the copying for this object
  virtual void Copy(wxShape& copy);

  // Does the copying for this object, including copying event
  // handler data if any. Calls the virtual Copy function.
  void CopyWithHandler(wxShape& copy);

  // Rotate about the given axis by the given amount in radians.
  virtual void Rotate(double x, double y, double theta);
  virtual double GetRotation() const { return m_rotation; }
  virtual void SetRotation(double rotation) { m_rotation = rotation; }

  void ClearAttachments();

  // Recentres all the text regions for this object
  void Recentre(wxDC& dc);

  // Clears points from a list of wxRealPoints
  void ClearPointList(wxList& list);

  // Return pen or brush of the right colour for the background
  wxPen GetBackgroundPen();
  wxBrush GetBackgroundBrush();


 protected:
  wxShapeEvtHandler*    m_eventHandler;
  bool                  m_formatted;
  double                m_xpos, m_ypos;
  const wxPen*          m_pen;
  const wxBrush*        m_brush;
  wxFont*               m_font;
  wxColour              m_textColour;
  wxString              m_textColourName;
  wxShapeCanvas*        m_canvas;
  wxList                m_lines;
  wxList                m_text;
  wxList                m_controlPoints;
  wxList                m_regions;
  wxList                m_attachmentPoints;
  bool                  m_visible;
  bool                  m_disableLabel;
  long                  m_id;
  bool                  m_selected;
  bool                  m_highlighted;      // Different from selected: user-defined highlighting,
                                            // e.g. thick border.
  double                m_rotation;
  int                   m_sensitivity;
  bool                  m_draggable;
  int                   m_attachmentMode;   // 0 for no attachments, 1 if using normal attachments,
                                            // 2 for branching attachments
  bool                  m_spaceAttachments; // true if lines at one side should be spaced
  bool                  m_fixedWidth;
  bool                  m_fixedHeight;
  bool                  m_centreResize;    // Default is to resize keeping the centre constant (true)
  bool                  m_drawHandles;     // Don't draw handles if false, usually true
  wxList                m_children;      // In case it's composite
  wxShape*              m_parent;      // In case it's a child
  int                   m_formatMode;
  int                   m_shadowMode;
  const wxBrush*        m_shadowBrush;
  int                   m_shadowOffsetX;
  int                   m_shadowOffsetY;
  int                   m_textMarginX;    // Gap between text and border
  int                   m_textMarginY;
  wxString              m_regionName;
  bool                  m_maintainAspectRatio;
  int                   m_branchNeckLength;
  int                   m_branchStemLength;
  int                   m_branchSpacing;
  long                  m_branchStyle;
};

class WXDLLIMPEXP_OGL wxPolygonShape: public wxShape
{
 DECLARE_DYNAMIC_CLASS(wxPolygonShape)
 public:
  wxPolygonShape();
  ~wxPolygonShape();

  // Takes a list of wxRealPoints; each point is an OFFSET from the centre.
  // Deletes user's points in destructor.
  virtual void Create(wxList *points);
  virtual void ClearPoints();

  void GetBoundingBoxMin(double *w, double *h) override;
  void CalculateBoundingBox();
  bool GetPerimeterPoint(double x1, double y1,
                         double x2, double y2,
                         double *x3, double *y3) override;
  bool HitTest(double x, double y, int *attachment, double *distance) override;
  void SetSize(double x, double y, bool recursive = true) override;
  void OnDraw(wxDC& dc) override;
  void OnDrawOutline(wxDC& dc, double x, double y, double w, double h) override;

  // Control points ('handles') redirect control to the actual shape, to make it easier
  // to override sizing behaviour.
  void OnSizingDragLeft(wxControlPoint* pt, bool draw, double x, double y, int keys=0, int attachment = 0) override;
  void OnSizingBeginDragLeft(wxControlPoint* pt, double x, double y, int keys=0, int attachment = 0) override;
  void OnSizingEndDragLeft(wxControlPoint* pt, double x, double y, int keys=0, int attachment = 0) override;

  // A polygon should have a control point at each vertex,
  // with the option of moving the control points individually
  // to change the shape.
  void MakeControlPoints() override;
  void ResetControlPoints() override;

  // If we've changed the shape, must make the original
  // points match the working points
  void UpdateOriginalPoints();

  // Add a control point after the given point
  virtual void AddPolygonPoint(int pos = 0);

  // Delete a control point
  virtual void DeletePolygonPoint(int pos = 0);

  // Recalculates the centre of the polygon
  virtual void CalculatePolygonCentre();

  int GetNumberOfAttachments() const override;
  bool GetAttachmentPosition(int attachment, double *x, double *y,
                             int nth = 0, int no_arcs = 1, wxLineShape *line = nullptr) override;
  bool AttachmentIsValid(int attachment) const override;
  // Does the copying for this object
  void Copy(wxShape& copy) override;

  inline wxList *GetPoints() { return m_points; }
  inline wxList *GetOriginalPoints() { return m_originalPoints; }

  // Rotate about the given axis by the given amount in radians
  void Rotate(double x, double y, double theta) override;

  double GetOriginalWidth() const { return m_originalWidth; }
  double GetOriginalHeight() const { return m_originalHeight; }

  void SetOriginalWidth(double w) { m_originalWidth = w; }
  void SetOriginalHeight(double h) { m_originalHeight = h; }

 private:
  wxList*       m_points;
  wxList*       m_originalPoints;
  double        m_boundWidth;
  double        m_boundHeight;
  double        m_originalWidth;
  double        m_originalHeight;
};

class WXDLLIMPEXP_OGL wxRectangleShape: public wxShape
{
 DECLARE_DYNAMIC_CLASS(wxRectangleShape)
 public:
  wxRectangleShape(double w = 0.0, double h = 0.0);
  void GetBoundingBoxMin(double *w, double *h) override;
  bool GetPerimeterPoint(double x1, double y1,
                         double x2, double y2,
                         double *x3, double *y3) override;
  void OnDraw(wxDC& dc) override;
  void SetSize(double x, double y, bool recursive = true) override;
  void SetCornerRadius(double rad); // If > 0, rounded corners
  double GetCornerRadius() const { return m_cornerRadius; }

  int GetNumberOfAttachments() const override;
  bool GetAttachmentPosition(int attachment, double *x, double *y,
                             int nth = 0, int no_arcs = 1, wxLineShape *line = nullptr) override;
  // Does the copying for this object
  void Copy(wxShape& copy) override;

  inline double GetWidth() const { return m_width; }
  inline double GetHeight() const { return m_height; }
  inline void SetWidth(double w) { m_width = w; }
  inline void SetHeight(double h) { m_height = h; }

protected:
  double m_width;
  double m_height;
  double m_cornerRadius;
};

class WXDLLIMPEXP_OGL wxTextShape: public wxRectangleShape
{
 DECLARE_DYNAMIC_CLASS(wxTextShape)
 public:
  wxTextShape(double width = 0.0, double height = 0.0);

  void OnDraw(wxDC& dc) override;

  // Does the copying for this object
  void Copy(wxShape& copy) override;
};

class WXDLLIMPEXP_OGL wxEllipseShape: public wxShape
{
 DECLARE_DYNAMIC_CLASS(wxEllipseShape)
 public:
  wxEllipseShape(double w = 0.0, double h = 0.0);

  void GetBoundingBoxMin(double *w, double *h) override;
  bool GetPerimeterPoint(double x1, double y1,
                         double x2, double y2,
                         double *x3, double *y3) override;

  void OnDraw(wxDC& dc) override;
  void SetSize(double x, double y, bool recursive = true) override;

  int GetNumberOfAttachments() const override;
  bool GetAttachmentPosition(int attachment, double *x, double *y,
                             int nth = 0, int no_arcs = 1, wxLineShape *line = nullptr) override;

  // Does the copying for this object
  void Copy(wxShape& copy) override;

  inline double GetWidth() const { return m_width; }
  inline double GetHeight() const { return m_height; }

  inline void SetWidth(double w) { m_width = w; }
  inline void SetHeight(double h) { m_height = h; }

protected:
  double m_width;
  double m_height;
};

class WXDLLIMPEXP_OGL wxCircleShape: public wxEllipseShape
{
 DECLARE_DYNAMIC_CLASS(wxCircleShape)
 public:
  wxCircleShape(double w = 0.0);

  bool GetPerimeterPoint(double x1, double y1,
                         double x2, double y2,
                         double *x3, double *y3) override;
  // Does the copying for this object
  void Copy(wxShape& copy) override;
};

#endif
 // _OGL_BASIC_H_
