/////////////////////////////////////////////////////////////////////////////
// Name:        coords.h
// Purpose:     Convert coords between pixels and points or twips
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef COORDS_H
#define COORDS_H

#include <limits>

/**
 * @file coords.h
 * @brief Convert coordinates between pixels and points or twips.
 */

namespace tt_solutions {

/**
 * @brief Convert coordinates between pixels and points or twips.
 */
template <int UnitsPerInch> class Coords
{
public:
    enum { Inch = UnitsPerInch };

    template <class T> static T FromPixels(T i, wxCoord dpi) {
        return (i * Inch + (isint(i) && i > 0 ? dpi - 1 : 0)) / dpi ;
    }
    template <class T> static T ToPixels(T i, wxCoord dpi) {
        return (i * dpi - (isint(i) && i < 0 ? Inch - 1 : 0)) / Inch;
    }
    template <class T> static T FromPixels(const T& pt, const wxSize& dpi) {
        return T(FromPixels(pt.x, dpi.x), FromPixels(pt.y, dpi.y));
    }
    template <class T> static T ToPixels(const T& pt, const wxSize& dpi) {
        return T(ToPixels(pt.x, dpi.x), ToPixels(pt.y, dpi.y));
    }

private:
    template <class T> static bool isint(T) {
        return std::numeric_limits<T>::is_integer;
    }
};

template <> class Coords<0>
{
public:
    enum { Inch = 0 };

    template <class T, class U> static T FromPixels(const T& i, const U&) {
        return i;
    }
    template <class T, class U> static T ToPixels(const T& i, const U&) {
        return i;
    }
};

typedef Coords<0> Pixels;
typedef Coords<72> Points;
typedef Coords<1440> Twips;

} // namespace tt_solutions

#endif // COORDS_H
