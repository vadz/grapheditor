/////////////////////////////////////////////////////////////////////////////
// Name:        tie.h
// Purpose:     Helper to assign a std::pair to two variables.
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef TIE_H
#define TIE_H

#include <utility>

/**
 * @file tie.h
 * @brief Helper to assign std::pair to two variables.
 */

namespace tt_solutions {

/*
 * Implementation classes
 */
namespace impl
{
    /**
     * @brief Implementation helper for tie().
     *
     * This class is never used directly but only as a return value of tie().
     * We must define our own class instead of simply using
     * <code>std::pair</code> to work around a well-known problem with using
     * reference types with the latter but the user code should remain unaware
     * of this.
     */
    template <class A, class B>
    struct RefPair
    {
        /// Constructor from pair components.
        RefPair(A& a, B& b) : first(a), second(b) { }

        /// Default copy constructor.
        RefPair(const RefPair<A, B>& t) = default;

        /// Assignment operator from a <code>std::pair</code>
        RefPair& operator=(const std::pair<A, B>& p)
        {
            first = p.first;
            second = p.second;
            return *this;
        }

        /// Default assignment operator.
        RefPair& operator=(const RefPair<A, B>& t) = default;

        A& first;       ///< First pair component.
        B& second;      ///< Second pair component.
    };

} // namespace impl

/**
 * @brief A helper to allow a <code>std::pair</code> to be assigned to two
 * variables.
 *
 * For example:
 * @code
 *  Graph::iterator it, end;
 *
 *  for (tie(it, end) = m_graph->GetSelection(); it != end; ++it)
 *      it->SetColour(colour);
 * @endcode
 */
template <class A, class B>
impl::RefPair<A, B> tie(A& a, B& b)
{
    return impl::RefPair<A, B>(a, b);
}

} // namespace tt_solutions

#endif // TIE_H
