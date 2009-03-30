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
 *
 * Defines the classes Pixels, Points and Twips.  When used to convert scaler
 * values the second parameter must be the screen DPI in the corresponding
 * direction. E.g.:
 * @code
 *  x = Pixels::From<Points>(x, xdpi);
 * @endcode
 *
 * Or they can be used to convert wxPoint, wxSize or wxRect values. In
 * this case the second parameter must be a wxSize giving screen DPI in
 * the x and y directions. E.g.:
 * @code
 *  rc = Pixels::To<Points>(rc, dpi);
 * @endcode
 *
 * For integer values <code>From</code> rounds down and <code>To</code> rounds
 * up.  Conversion to higher resolution coordinates and back is a bijection
 * assuming <code>From</code> is used one way and <code>To</code> the other.
 */
template <int U> class Coords
{
public:
    /**
     * @brief For Points 72, for Twips 1440, or the special value 0 for
     * Pixels.
     */
    enum { Units = U };
    static const double Inch;

    /**
     * @brief Convert a scalar value.
     *
     * For example:
     * @code
     * x = Pixels::From<Points>(x, xdpi);
     * @endcode
     *
     * @param i An integer or floating point x or y value.
     * @param dpi An integer giving the DPI in the correspoinding x or y
     * direction.
     *
     * When i is integer <code>From</code> rounds <em>down</em>.
     */
    template <class C, class T> static T From(T i, int dpi) {
        return Trans<Units, C::Units, T>::From(i, dpi);
    }
    /**
     * @brief Convert a scalar value.
     *
     * For example:
     * @code
     * x = Pixels::To<Points>(x, xdpi);
     * @endcode
     *
     * @param i An integer or floating point x or y value.
     * @param dpi An integer giving the DPI in the correspoinding x or y
     * direction.
     *
     * When i is integer <code>To</code> rounds <em>up</em>.
     */
    template <class C, class T> static T To(T i, int dpi) {
        return Trans<Units, C::Units, T>::To(i, dpi);
    }

    /**
     * @brief Convert a wxPoint or a wxSize value.
     *
     * For example:
     * @code
     * pt = Pixels::From<Points>(pt, dpi);
     * @endcode
     *
     * @param pt A wxPoint or wxSize value.
     * @param dpi A wxSize giving the DPIs in the x and y directions.
     *
     * <code>From</code> rounds <em>down</em>.
     */
    template <class C, class T> static T From(const T& pt, const wxSize& dpi) {
        return T(From<C>(pt.x, dpi.x), From<C>(pt.y, dpi.y));
    }
    /**
     * @brief Convert a wxPoint or a wxSize value.
     *
     * For example:
     * @code
     * pt = Pixels::To<Points>(pt, dpi);
     * @endcode
     *
     * @param pt A wxPoint or wxSize value.
     * @param dpi A wxSize giving the DPIs in the x and y directions.
     *
     * <code>To</code> rounds <em>up</em>.
     */
    template <class C, class T> static T To(const T& pt, const wxSize& dpi) {
        return T(To<C>(pt.x, dpi.x), To<C>(pt.y, dpi.y));
    }

    /**
     * @brief Convert a wxRect value.
     *
     * For example:
     * @code
     * rc = Pixels::From<Points>(rc, dpi);
     * @endcode
     *
     * @param rc A wxRect value.
     * @param dpi A wxSize giving the DPIs in the x and y directions.
     *
     * <code>From</code> rounds <em>down</em>.
     */
    template <class C> static wxRect From(const wxRect& rc, const wxSize& dpi) {
        return wxRect(From<C>(rc.GetPosition(), dpi),
                      From<C>(rc.GetSize(), dpi));
    }
    /**
     * @brief Convert a wxRect value.
     *
     * For example:
     * @code
     * rc = Pixels::To<Points>(rc, dpi);
     * @endcode
     *
     * @param rc A wxRect value.
     * @param dpi A wxSize giving the DPIs in the x and y directions.
     *
     * <code>To</code> rounds <em>up</em>.
     */
    template <class C> static wxRect To(const wxRect& rc, const wxSize& dpi) {
        return wxRect(To<C>(rc.GetPosition(), dpi),
                      To<C>(rc.GetSize(), dpi));
    }

private:
    template <class T, bool isint>
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
        enum { isint = std::numeric_limits<T>::is_integer };

        static T From(T i, int)    { return Trans0<T, isint>::From(i, I1, I2); }
        static T To(T i, int)      { return Trans0<T, isint>::To(i, I1, I2); }
    };

    template <int I2, class T> struct Trans<0, I2, T>
    {
        enum { isint = std::numeric_limits<T>::is_integer };

        static T From(T i, int I1) { return Trans0<T, isint>::From(i, I1 * 10, I2); }
        static T To(T i, int I1)   { return Trans0<T, isint>::To(i, I1 * 10, I2); }
    };

    template <int I1, class T> struct Trans<I1, 0, T>
    {
        enum { isint = std::numeric_limits<T>::is_integer };

        static T From(T i, int I2) { return Trans0<T, isint>::From(i, I1, I2 * 10); }
        static T To(T i, int I2)   { return Trans0<T, isint>::To(i, I1, I2 * 10); }
    };

    template <class T> struct Trans<0, 0, T>
    {
        static T From(T i, int)     { return i; }
        static T To(T i, int)       { return i; }
    };
};

template <int U> const double Coords<U>::Inch = U / 10.0;

typedef Coords<0> Pixels;
typedef Coords<10> Inches;
typedef Coords<254> MM;
typedef Coords<720> Points;
typedef Coords<14400> Twips;

} // namespace tt_solutions

#endif // COORDS_H
