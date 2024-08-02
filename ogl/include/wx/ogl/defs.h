///////////////////////////////////////////////////////////////////////////////
// Name:        wx/ogl/defs.h
// Purpose:     Common definitions used in the other wxOGL headers.
// Author:      Vadim Zeitlin
// Created:     2024-04-23
// Copyright:   (c) 2024 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_OGL_DEFS_H_
#define _WX_OGL_DEFS_H_

#ifdef WXMAKINGDLL_OGL
    #define WXDLLIMPEXP_OGL WXEXPORT
#elif defined(WXUSINGDLL_OGL)
    #define WXDLLIMPEXP_OGL WXIMPORT
#else // not making nor using DLL
    #define WXDLLIMPEXP_OGL
#endif

#include <wx/dcclient.h>

#include <vector>

using wxOGLPoints = std::vector<wxRealPoint>;

#ifndef wxHAS_INFO_DC
    using wxReadOnlyDC = wxDC;
    using wxInfoDC = wxClientDC;
#endif

#endif // _WX_OGL_DEFS_H_
