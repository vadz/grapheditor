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

// There are a few numbers used here to define the conversion factors between
// units and it doesn't seem worth to define separate constants for them, so
// just suppress clang-tidy warnings about them.
//
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

namespace tt_solutions {

/**
 * @brief Convert coordinates between pixels and points or twips.
 *
 * Defines the classes Pixels, Points and Twips.  When used to convert scalar
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
    /** @cond */
    enum { Units = U };
    /** @endcond */
    /**
     * @brief For Points 72, for Twips 1440, or the special value 0 for
     * Pixels.
     */
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
     * @param dpi An integer giving the DPI in the corresponding x or y
     * direction.
     *
     * When i is integer <code>%From()</code> rounds <em>down</em>.
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
     * @param dpi An integer giving the DPI in the corresponding x or y
     * direction.
     *
     * When i is integer <code>%To()</code> rounds <em>up</em>.
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
     * <code>%From()</code> rounds <em>down</em>.
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
     * <code>%To()</code> rounds <em>up</em>.
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
     * <code>%From()</code> rounds <em>down</em>.
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
     * <code>%To()</code> rounds <em>up</em>.
     */
    template <class C> static wxRect To(const wxRect& rc, const wxSize& dpi) {
        return wxRect(To<C>(rc.GetPosition(), dpi),
                      To<C>(rc.GetSize(), dpi));
    }

private:
    /**
     * @brief Generic implementation of the coordinates transformation.
     *
     * This implementation is only used for floating point types.
     */
    template <class T, bool isint>
    struct Trans0
    {
        /// Convert an amount in @a I2 units to @a I1 units.
        static T From(T i, int I1, int I2) { return i * I1 / I2; }

        /// Convert an amount in @a I1 units to @a I2 units.
        static T To(T i, int I1, int I2)   { return i * I2 / I1; }
    };

    /**
     * @brief Specialization of the transformation for the integer ratios.
     *
     * This specialization uses integer division when possible and ensures that
     * the result is rounded down.
     */
    template <class T>
    struct Trans0<T, true>
    {
        /// Convert an amount in @a I2 units to @a I1 units.
        static T From(T i, int I1, int I2) {
            return (i * I1 - (i < 0 ? I2 - 1 : 0)) / I2;
        }

        /// Convert an amount in @a I1 units to @a I2 units.
        static T To(T i, int I1, int I2) {
            return (i * I2 + (i > 0 ? I1 - 1 : 0)) / I1;
        }
    };

    /**
     * @brief Transform between two non-pixel units.
     *
     * See the specializations below for the pixel-specific conversions.
     */
    template <int I1, int I2, class T> struct Trans
    {
        enum { isint = std::numeric_limits<T>::is_integer };

        /// Convert an amount in @a I2 units to @a I1 units.
        static T From(T i, int)    { return Trans0<T, isint>::From(i, I1, I2); }

        /// Convert an amount in @a I1 units to @a I2 units.
        static T To(T i, int)      { return Trans0<T, isint>::To(i, I1, I2); }
    };

    /**
     * @brief Transform between pixels and a non-pixel unit.
     *
     * This is a specialization of the above template for the first unit being
     * 0, i.e. pixels. Notice that as the other units are expressed in tenths
     * of an inch we need an extra factor of 10 in this case.
     */
    template <int I2, class T> struct Trans<0, I2, T>
    {
        enum { isint = std::numeric_limits<T>::is_integer };

        /// Convert an amount in @a I2 units to @a I1 units.
        static T From(T i, int I1) { return Trans0<T, isint>::From(i, I1 * 10, I2); }

        /// Convert an amount in @a I1 units to @a I2 units.
        static T To(T i, int I1)   { return Trans0<T, isint>::To(i, I1 * 10, I2); }
    };

    /**
     * @brief Transform between a non-pixel unit and pixels.
     *
     * This is a symmetric version of the specialization above.
     */
    template <int I1, class T> struct Trans<I1, 0, T>
    {
        enum { isint = std::numeric_limits<T>::is_integer };

        /// Convert an amount in pixels to @a I1 units.
        static T From(T i, int I2) { return Trans0<T, isint>::From(i, I1, I2 * 10); }

        /// Convert an amount in @a I1 units to pixels.
        static T To(T i, int I2)   { return Trans0<T, isint>::To(i, I1, I2 * 10); }
    };

    /**
     * @brief Transform between pixels and pixels.
     *
     * This is a trivial specialization for the identity conversion between
     * pixels.
     */
    template <class T> struct Trans<0, 0, T>
    {
        /// Identical transformation from pixels to pixels.
        static T From(T i, int)     { return i; }

        /// Identical transformation from pixels to pixels.
        static T To(T i, int)       { return i; }
    };
};

template <int U> const double Coords<U>::Inch = U / 10.0;

/**
 * Pixel coordinates class.
 *
 * The unit of pixels is, by convention, 0. This is used by Coords class and
 * shouldn't be changed.
 */
typedef Coords<0> Pixels;

/**
 * Inch coordinates class.
 *
 * To minimize rounding errors, we use one tenth of an inch as a base factor,
 * hence the factor for this class is 10 and not 1.
 */
typedef Coords<10> Inches;

/**
 * Millimetre coordinates class.
 *
 * One inch is 2.54cm or 25.4mm and, as we use one tenth of an inch as a base
 * factor, this class is defined with a factor of 254.
 */
typedef Coords<254> MM;

/**
 * Point coordinates class.
 *
 * One inch is, by definition of a point, 72pp.
 */
typedef Coords<720> Points;

/**
 * Twip coordinate class.
 *
 * One twip, being "TWentieth of an Inch Point", is 1/1440 of an inch.
 */
typedef Coords<14400> Twips;

} // namespace tt_solutions

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

#endif // COORDS_H
