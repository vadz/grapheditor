/////////////////////////////////////////////////////////////////////////////
// Name:        divided.h
// Purpose:     wxDividedShape
// Author:      Julian Smart
// Modified by:
// Created:     12/07/98
// RCS-ID:      $Id$
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _OGL_DIVIDED_H_
#define _OGL_DIVIDED_H_


/*
 * Definition of a region
 *
 */

/*
 * Box divided into horizontal regions
 *
 */

extern wxFont *g_oglNormalFont;
class WXDLLIMPEXP_OGL wxDividedShape: public wxRectangleShape
{
 DECLARE_DYNAMIC_CLASS(wxDividedShape)

 public:
  wxDividedShape(double w = 0.0, double h = 0.0);
  ~wxDividedShape();

  void OnDraw(wxDC& dc) override;
  void OnDrawContents(wxDC& dc) override;

  void SetSize(double w, double h, bool recursive = true) override;

  void MakeControlPoints() override;
  void ResetControlPoints() override;

  void MakeMandatoryControlPoints() override;
  void ResetMandatoryControlPoints() override;

  void Copy(wxShape &copy) override;

  // Set all region sizes according to proportions and
  // this object total size
  void SetRegionSizes();

  // Edit region colours/styles
  void EditRegions();

  // Attachment points correspond to regions in the divided box
  bool GetAttachmentPosition(int attachment, double *x, double *y,
                             int nth = 0, int no_arcs = 1, wxLineShape *line = nullptr) override;
  bool AttachmentIsValid(int attachment) const override;
  int GetNumberOfAttachments() const override;

  // Invoke editor on CTRL-right click
  void OnRightClick(double x, double y, int keys = 0, int attachment = 0) override;
};

#endif
    // _OGL_DIVIDED_H_

