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

#include <wx/wx.h>
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

    template <class C, class T> static T From(T i, int dpi) {
        return Trans<Inch, C::Inch, T>::From(i, dpi);
    }
    template <class C, class T> static T To(T i, int dpi) {
        return Trans<Inch, C::Inch, T>::To(i, dpi);
    }

    template <class C, class T> static T From(const T& pt, const wxSize& dpi) {
        return T(From<C>(pt.x, dpi.x), From<C>(pt.y, dpi.y));
    }
    template <class C, class T> static T To(const T& pt, const wxSize& dpi) {
        return T(To<C>(pt.x, dpi.x), To<C>(pt.y, dpi.y));
    }

    template <class C>
    static wxRect From(const wxRect& rect, const wxSize& dpi) {
        return wxRect(From<C>(rect.GetPosition(), dpi),
                      From<C>(rect.GetSize(), dpi));
    }
    template <class C>
    static wxRect To(const wxRect& rect, const wxSize& dpi) {
        return wxRect(To<C>(rect.GetPosition(), dpi),
                      To<C>(rect.GetSize(), dpi));
    }

private:
    template <class T, bool isint = std::numeric_limits<T>::is_integer>
    struct Trans0
    {
        static T From(T i, int I1, int I2) { return i * I1 / I2; }
        static T To(T i, int I1, int I2)   { return i * I2 / I1; }
    };

    template <class T>
    struct Trans0<T, true>
    {
        static T From(T i, int I1, int I2) {
            return (i * I1 - (i < 0 ? I2 - 1 : 0)) / I2;
        }
        static T To(T i, int I1, int I2) {
            return (i * I2 + (i > 0 ? I1 - 1 : 0)) / I1;
        }
    };

    template <int I1, int I2, class T> struct Trans
    {
        static T From(T i, int)     { return Trans0<T>::From(i, I1, I2); }
        static T To(T i, int)       { return Trans0<T>::To(i, I1, I2); }
    };

    template <int I2, class T> struct Trans<0, I2, T>
    {
        static T From(T i, int I1)  { return Trans0<T>::From(i, I1, I2); }
        static T To(T i, int I1)    { return Trans0<T>::To(i, I1, I2); }
    };

    template <int I1, class T> struct Trans<I1, 0, T>
    {
        static T From(T i, int I2)  { return Trans0<T>::From(i, I1, I2); }
        static T To(T i, int I2)    { return Trans0<T>::To(i, I1, I2); }
    };

    template <class T> struct Trans<0, 0, T>
    {
        static T From(T i, int)     { return i; }
        static T To(T i, int)       { return i; }
    };
};

typedef Coords<0> Pixels;
typedef Coords<72> Points;
typedef Coords<1440> Twips;

} // namespace tt_solutions

#endif // COORDS_H
