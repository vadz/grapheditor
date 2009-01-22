/////////////////////////////////////////////////////////////////////////////
// Name:        tie.h
// Purpose:     Helper to assign a std::pair to two variables.
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
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

    template <class A, class B>
    struct RefPair
    {
        RefPair(A& a, B& b) : first(a), second(b) { }

        RefPair& operator=(const std::pair<A, B>& p)
        {
            first = p.first;
            second = p.second;
            return *this;
        }

        RefPair& operator=(const RefPair<A, B>& t)
        {
            first = t.first;
            second = t.second;
            return *this;
        }

        A& first;
        B& second;
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
