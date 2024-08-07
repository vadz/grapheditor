/////////////////////////////////////////////////////////////////////////////
// Name:        canvas.cpp
// Purpose:     Shape canvas class
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

#define CONTROL_POINT_SIZE       6

// Control point types
// Rectangle and most other shapes
#define CONTROL_POINT_VERTICAL   1
#define CONTROL_POINT_HORIZONTAL 2
#define CONTROL_POINT_DIAGONAL   3

// Line
#define CONTROL_POINT_ENDPOINT_TO 4
#define CONTROL_POINT_ENDPOINT_FROM 5
#define CONTROL_POINT_LINE       6

IMPLEMENT_DYNAMIC_CLASS(wxShapeCanvas, wxScrolledWindow)

BEGIN_EVENT_TABLE(wxShapeCanvas, wxScrolledWindow)
    EVT_PAINT(wxShapeCanvas::OnPaint)
    EVT_MOUSE_EVENTS(wxShapeCanvas::OnMouseEvent)
    EVT_MOUSE_CAPTURE_LOST(wxShapeCanvas::OnCaptureLost)
END_EVENT_TABLE()

const wxChar* wxShapeCanvasNameStr = wxT("shapeCanvas");

// Object canvas
wxShapeCanvas::wxShapeCanvas(wxWindow *parent, wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxString& name):
  wxScrolledWindow(parent, id, pos, size, style, name)
{
  m_shapeDiagram = nullptr;
  m_dragState = NoDragging;
  m_draggedShape = nullptr;
  m_oldDragX = 0;
  m_oldDragY = 0;
  m_firstDragX = 0;
  m_firstDragY = 0;
  m_checkTolerance = true;

  SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
}

wxShapeCanvas::~wxShapeCanvas()
{
}

void wxShapeCanvas::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);

    PrepareDC(dc);

    if (GetDiagram())
        GetDiagram()->Redraw(dc);
}

void wxShapeCanvas::OnMouseEvent(wxMouseEvent& event)
{
  if (!GetDiagram())
    return;

  wxInfoDC dc(this);
  PrepareDC(dc);

  wxPoint logPos(event.GetLogicalPosition(dc));

  double x, y;
  x = (double) logPos.x;
  y = (double) logPos.y;

  int keys = 0;
  if (event.ShiftDown())
    keys = keys | KEY_SHIFT;
  if (event.ControlDown())
    keys = keys | KEY_CTRL;

  bool dragging = event.Dragging();

  // Check if we're within the tolerance for mouse movements.
  // If we're very close to the position we started dragging
  // from, this may not be an intentional drag at all.
  if (dragging)
  {
    int dx = abs(dc.LogicalToDeviceXRel((long) (x - m_firstDragX)));
    int dy = abs(dc.LogicalToDeviceYRel((long) (y - m_firstDragY)));
    if (m_checkTolerance && (dx <= GetDiagram()->GetMouseTolerance()) && (dy <= GetDiagram()->GetMouseTolerance()))
    {
      return;
    }
    else
      // If we've ignored the tolerance once, then ALWAYS ignore
      // tolerance in this drag, even if we come back within
      // the tolerance range.
      m_checkTolerance = false;
  }

  // Dragging - note that the effect of dragging is left entirely up
  // to the object, so no movement is done unless explicitly done by
  // object.
  if (dragging && m_draggedShape && m_dragState == StartDraggingLeft)
  {
    m_dragState = ContinueDraggingLeft;

    // If the object isn't m_draggable, transfer message to canvas
    if (m_draggedShape->Draggable())
      m_draggedShape->GetEventHandler()->OnBeginDragLeft((double)x, (double)y, keys, m_draggedAttachment);
    else
    {
      m_draggedShape = nullptr;
      OnBeginDragLeft((double)x, (double)y, keys);
    }

    m_oldDragX = x; m_oldDragY = y;
  }
  else if (dragging && m_draggedShape && m_dragState == ContinueDraggingLeft)
  {
    // Continue dragging
    m_draggedShape->GetEventHandler()->OnDragLeft(false, m_oldDragX, m_oldDragY, keys, m_draggedAttachment);
    m_draggedShape->GetEventHandler()->OnDragLeft(true, (double)x, (double)y, keys, m_draggedAttachment);
    m_oldDragX = x; m_oldDragY = y;
  }
  else if (event.LeftUp() && m_draggedShape && m_dragState == ContinueDraggingLeft)
  {
    m_dragState = NoDragging;
    m_checkTolerance = true;

    m_draggedShape->GetEventHandler()->OnDragLeft(false, m_oldDragX, m_oldDragY, keys, m_draggedAttachment);

    m_draggedShape->GetEventHandler()->OnEndDragLeft((double)x, (double)y, keys, m_draggedAttachment);
    m_draggedShape = nullptr;
  }
  else if (dragging && m_draggedShape && m_dragState == StartDraggingRight)
  {
    m_dragState = ContinueDraggingRight;

    if (m_draggedShape->Draggable())
      m_draggedShape->GetEventHandler()->OnBeginDragRight((double)x, (double)y, keys, m_draggedAttachment);
    else
    {
      m_draggedShape = nullptr;
      OnBeginDragRight((double)x, (double)y, keys);
    }
    m_oldDragX = x; m_oldDragY = y;
  }
  else if (dragging && m_draggedShape && m_dragState == ContinueDraggingRight)
  {
    // Continue dragging
    m_draggedShape->GetEventHandler()->OnDragRight(false, m_oldDragX, m_oldDragY, keys, m_draggedAttachment);
    m_draggedShape->GetEventHandler()->OnDragRight(true, (double)x, (double)y, keys, m_draggedAttachment);
    m_oldDragX = x; m_oldDragY = y;
  }
  else if (event.RightUp() && m_draggedShape && m_dragState == ContinueDraggingRight)
  {
    m_dragState = NoDragging;
    m_checkTolerance = true;

    m_draggedShape->GetEventHandler()->OnDragRight(false, m_oldDragX, m_oldDragY, keys, m_draggedAttachment);

    m_draggedShape->GetEventHandler()->OnEndDragRight((double)x, (double)y, keys, m_draggedAttachment);
    m_draggedShape = nullptr;
  }

  // All following events sent to canvas, not object
  else if (dragging && !m_draggedShape && m_dragState == StartDraggingLeft)
  {
    m_dragState = ContinueDraggingLeft;
    OnBeginDragLeft((double)x, (double)y, keys);
    m_oldDragX = x; m_oldDragY = y;
  }
  else if (dragging && !m_draggedShape && m_dragState == ContinueDraggingLeft)
  {
    // Continue dragging
    OnDragLeft(false, m_oldDragX, m_oldDragY, keys);
    OnDragLeft(true, (double)x, (double)y, keys);
    m_oldDragX = x; m_oldDragY = y;
  }
  else if (event.LeftUp() && !m_draggedShape && m_dragState == ContinueDraggingLeft)
  {
    m_dragState = NoDragging;
    m_checkTolerance = true;

    OnDragLeft(false, m_oldDragX, m_oldDragY, keys);
    OnEndDragLeft((double)x, (double)y, keys);
    m_draggedShape = nullptr;
  }
  else if (dragging && !m_draggedShape && m_dragState == StartDraggingRight)
  {
    m_dragState = ContinueDraggingRight;
    OnBeginDragRight((double)x, (double)y, keys);
    m_oldDragX = x; m_oldDragY = y;
  }
  else if (dragging && !m_draggedShape && m_dragState == ContinueDraggingRight)
  {
    // Continue dragging
    OnDragRight(false, m_oldDragX, m_oldDragY, keys);
    OnDragRight(true, (double)x, (double)y, keys);
    m_oldDragX = x; m_oldDragY = y;
  }
  else if (event.RightUp() && !m_draggedShape && m_dragState == ContinueDraggingRight)
  {
    m_dragState = NoDragging;
    m_checkTolerance = true;

    OnDragRight(false, m_oldDragX, m_oldDragY, keys);
    OnEndDragRight((double)x, (double)y, keys);
    m_draggedShape = nullptr;
  }

  // Non-dragging events
  else if (event.IsButton())
  {
    m_checkTolerance = true;

    // Find the nearest object
    int attachment = 0;
    wxShape *nearest_object = FindShape(x, y, &attachment);
    if (nearest_object) // Object event
    {
      if (event.LeftDown())
      {
        m_draggedShape = nearest_object;
        m_draggedAttachment = attachment;
        m_dragState = StartDraggingLeft;
        m_firstDragX = x;
        m_firstDragY = y;
      }
      else if (event.LeftUp())
      {
        // N.B. Only register a click if the same object was
        // identified for down *and* up.
        if (nearest_object == m_draggedShape)
          nearest_object->GetEventHandler()->OnLeftClick((double)x, (double)y, keys, attachment);

        m_draggedShape = nullptr;
        m_dragState = NoDragging;
      }
      else if (event.LeftDClick())
      {
        nearest_object->GetEventHandler()->OnLeftDoubleClick((double)x, (double)y, keys, attachment);

        m_draggedShape = nullptr;
        m_dragState = NoDragging;
      }
      else if (event.RightDown())
      {
        m_draggedShape = nearest_object;
        m_draggedAttachment = attachment;
        m_dragState = StartDraggingRight;
        m_firstDragX = x;
        m_firstDragY = y;
      }
      else if (event.RightUp())
      {
        if (nearest_object == m_draggedShape)
          nearest_object->GetEventHandler()->OnRightClick((double)x, (double)y, keys, attachment);

        m_draggedShape = nullptr;
        m_dragState = NoDragging;
      }
    }
    else // Canvas event (no nearest object)
    {
      if (event.LeftDown())
      {
        m_draggedShape = nullptr;
        m_dragState = StartDraggingLeft;
        m_firstDragX = x;
        m_firstDragY = y;
      }
      else if (event.LeftUp())
      {
        OnLeftClick((double)x, (double)y, keys);

        m_draggedShape = nullptr;
        m_dragState = NoDragging;
      }
      else if (event.RightDown())
      {
        m_draggedShape = nullptr;
        m_dragState = StartDraggingRight;
        m_firstDragX = x;
        m_firstDragY = y;
      }
      else if (event.RightUp())
      {
        OnRightClick((double)x, (double)y, keys);

        m_draggedShape = nullptr;
        m_dragState = NoDragging;
      }
    }
  }
}

void wxShapeCanvas::EndDrag()
{
    // Make sure we relinquish the mouse capture if we have it.
    if ( HasCapture() )
        ReleaseMouse();

    // And erase everything we could have drawn while dragging.
    ClearHints();
}

void wxShapeCanvas::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
    EndDrag();
}

/*
 * Try to find a sensitive object, working up the hierarchy of composites.
 *
 */
wxShape *wxShapeCanvas::FindFirstSensitiveShape(double x, double y, int *new_attachment, int op)
{
  wxShape *image = FindShape(x, y, new_attachment);
  if (!image) return nullptr;

  wxShape *actualImage = FindFirstSensitiveShape1(image, op);
  if (actualImage)
  {
    double dist;
    // Find actual attachment
    actualImage->HitTest(x, y, new_attachment, &dist);
  }
  return actualImage;
}

wxShape *wxShapeCanvas::FindFirstSensitiveShape1(wxShape *image, int op)
{
  if (image->GetSensitivityFilter() & op)
    return image;
  if (image->GetParent())
    return FindFirstSensitiveShape1(image->GetParent(), op);
  return nullptr;
}

// Helper function: true if 'contains' wholly contains 'contained'.
static bool WhollyContains(wxShape *contains, wxShape *contained)
{
  double xp1, yp1, xp2, yp2;
  double w1, h1, w2, h2;
  double left1, top1, right1, bottom1, left2, top2, right2, bottom2;

  xp1 = contains->GetX(); yp1 = contains->GetY(); xp2 = contained->GetX(); yp2 = contained->GetY();
  contains->GetBoundingBoxMax(&w1, &h1);
  contained->GetBoundingBoxMax(&w2, &h2);

  left1 = (double)(xp1 - (w1 / 2.0));
  top1 = (double)(yp1 - (h1 / 2.0));
  right1 = (double)(xp1 + (w1 / 2.0));
  bottom1 = (double)(yp1 + (h1 / 2.0));

  left2 = (double)(xp2 - (w2 / 2.0));
  top2 = (double)(yp2 - (h2 / 2.0));
  right2 = (double)(xp2 + (w2 / 2.0));
  bottom2 = (double)(yp2 + (h2 / 2.0));

  return ((left1 <= left2) && (top1 <= top2) && (right1 >= right2) && (bottom1 >= bottom2));
}

wxShape *wxShapeCanvas::FindShape(double x, double y, int *attachment, wxClassInfo *info, wxShape *notObject)
{
  if (!GetDiagram())
    return nullptr;

  double nearest = 100000.0;
  int nearest_attachment = 0;
  wxShape *nearest_object = nullptr;

  // Go backward through the object list, since we want:
  // (a) to have the control points drawn LAST to overlay
  //     the other objects
  // (b) to find the control points FIRST if they exist

  wxObjectList::compatibility_iterator current = GetDiagram()->GetShapeList()->GetLast();
  while (current)
  {
    wxShape *object = (wxShape *)current->GetData();

    double dist;
    int temp_attachment;

    // First pass for lines, which might be inside a container, so we
    // want lines to take priority over containers. This first loop
    // could fail if we clickout side a line, so then we'll
    // try other shapes.
    if (object->IsShown() &&
        object->IsKindOf(CLASSINFO(wxLineShape)) &&
        object->HitTest(x, y, &temp_attachment, &dist) &&
        ((info == nullptr) || object->IsKindOf(info)) &&
        (!notObject || !notObject->HasDescendant(object)))
    {
      // A line is trickier to spot than a normal object.
      // For a line, since it's the diagonal of the box
      // we use for the hit test, we may have several
      // lines in the box and therefore we need to be able
      // to specify the nearest point to the centre of the line
      // as our hit criterion, to give the user some room for
      // manouevre.
      if (dist < nearest)
      {
        nearest = dist;
        nearest_object = object;
        nearest_attachment = temp_attachment;
      }
    }
    if (current)
      current = current->GetPrevious();
  }

  current = GetDiagram()->GetShapeList()->GetLast();
  while (current)
  {
    wxShape *object = (wxShape *)current->GetData();
    double dist;
    int temp_attachment;

    // On second pass, only ever consider non-composites or divisions. If children want to pass
    // up control to the composite, that's up to them.
    if (object->IsShown() && (object->IsKindOf(CLASSINFO(wxDivisionShape)) || !object->IsKindOf(CLASSINFO(wxCompositeShape)))
        && object->HitTest(x, y, &temp_attachment, &dist) && ((info == nullptr) || object->IsKindOf(info)) &&
        (!notObject || !notObject->HasDescendant(object)))
    {
      if (!object->IsKindOf(CLASSINFO(wxLineShape)))
      {
        // If we've hit a container, and we have already found a line in the
        // first pass, then ignore the container in case the line is in the container.
        // Check for division in case line straddles divisions (i.e. is not wholly contained).
        if (!nearest_object || !(object->IsKindOf(CLASSINFO(wxDivisionShape)) || WhollyContains(object, nearest_object)))
        {
          nearest_object = object;
          nearest_attachment = temp_attachment;
          current = GetDiagram()->GetShapeList()->GetFirst()->GetPrevious(); // finish loop
        }
      }
    }
    if (current)
      current = current->GetPrevious();
  }

  *attachment = nearest_attachment;
  return nearest_object;
}

/*
 * Higher-level events called by OnEvent
 *
 */

void wxShapeCanvas::OnLeftClick(double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::OnRightClick(double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::OnDragLeft(bool WXUNUSED(draw), double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::OnBeginDragLeft(double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::OnEndDragLeft(double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::OnDragRight(bool WXUNUSED(draw), double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::OnBeginDragRight(double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::OnEndDragRight(double WXUNUSED(x), double WXUNUSED(y), int WXUNUSED(keys))
{
}

void wxShapeCanvas::AddShape(wxShape *object, wxShape *addAfter)
 { GetDiagram()->AddShape(object, addAfter); }
void wxShapeCanvas::InsertShape(wxShape *object)
 { GetDiagram()->InsertShape(object); }
void wxShapeCanvas::RemoveShape(wxShape *object)
 { GetDiagram()->RemoveShape(object); }
void wxShapeCanvas::Redraw(wxDC& dc)
 { GetDiagram()->Redraw(dc); }
void wxShapeCanvas::Snap(double *x, double *y)
 { GetDiagram()->Snap(x, y); }
