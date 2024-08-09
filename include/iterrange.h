/////////////////////////////////////////////////////////////////////////////
// Name:        iterrange.h
// Purpose:     Helper for working with pairs of iterators defining a range.
// Author:      Vadim Zeitlin
// Created:     2024-08-06
// Copyright:   (c) 2024 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef ITERRANGE_H
#define ITERRANGE_H

#include <utility>

/**
 * @file iterrange.h
 * @brief Helper for working with pairs of iterators defining a range.
 *
 * Not all pairs of iterators necessarily define a range, e.g. only
 * equal_range() returns values representing a valid range out of multiple
 * functions returning pairs of iterators in the standard library, but many
 * functions in our library do, and the helper MakeRange() function defined
 * here makes it much more convenient to work with them.
 */

namespace tt_solutions {

/**
 * @brief Implementation helper for MakeRange().
 *
 * This class is not used directly but only as a return value of
 * MakeRange().
 */
template <class I>
struct IterRange : std::pair<I, I>
{
    using std::pair<I, I>::pair;

    /**
     * Return the first iterator of the range.
     *
     * Together with end(), this allows to use the range in a range-based
     * for loop.
     */
    I begin() { return this->first; }

    /**
     * Return the second iterator of the range.
     *
     * Together with begin(), this allows to use the range in a range-based
     * for loop.
     */
    I end() { return this->second; }
};

/**
 * @brief A helper to allow creating an iterable range from a pair of iterators.
 *
 * For example:
 * @code
 *  for (auto& it: MakeRange(m_graph->GetSelection()))
 *      it->SetColour(colour);
 * @endcode
 */
template <class I>
IterRange<I> MakeRange(const std::pair<I, I>& p) {
    // Note that due to const-related restrictions, we can't use the ctor from
    // pair itself here directly.
    return IterRange<I>(p.first, p.second);
}

} // namespace tt_solutions

#endif // ITERRANGE_H
