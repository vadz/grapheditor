/////////////////////////////////////////////////////////////////////////////
// Name:        factory.cpp
// Purpose:     Object Factory
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "factory.h"

/**
 * @file
 * @brief Implementation of FactoryBase class.
 */

namespace tt_solutions {

namespace impl {

/// Factories registry allowing fast access by string key.
using ClassMap = std::unordered_map<wxString, FactoryBase *>;

/// Registry indexing factories by their type name from type info.
ClassMap& GetIndexByType()
{
    static ClassMap s_typeidx;
    return s_typeidx;
}

/// Registry indexing factories by their name as specified in ctor.
ClassMap& GetIndexByName()
{
    static ClassMap s_nameidx;
    return s_nameidx;
}

void FactoryBase::Register()
{
    FactoryBase*& p1 = GetIndexByType()[m_type];
    FactoryBase*& p2 = GetIndexByName()[m_name];

    wxASSERT(p1 == NULL);
    wxASSERT_MSG(p2 == NULL,
        _T("Factory::Impl defined with duplicate name string"));

    p1 = p2 = this;
}

void FactoryBase::Unregister()
{
    int n1 = GetIndexByType().erase(m_type);
    int n2 = GetIndexByName().erase(m_name);
    wxASSERT(n1 == 1 && n2 == 1);
    (void)n1;
    (void)n2;
}

FactoryBase *FactoryBase::Get(const std::type_info& type)
{
    auto& typeidx = GetIndexByType();

    if (!typeidx.empty()) {
        wxString key = wxString::FromAscii(type.name());
        ClassMap::const_iterator it = typeidx.find(key);

        if (it != typeidx.end())
            return it->second;
    }

    return NULL;
}

FactoryBase *FactoryBase::Get(const wxString& name)
{
    auto& nameidx = GetIndexByName();

    if (!nameidx.empty()) {
        ClassMap::const_iterator it = nameidx.find(name);

        if (it != nameidx.end())
            return it->second;
    }

    return NULL;
}

} // namespace impl

} // namespace tt_solutions
