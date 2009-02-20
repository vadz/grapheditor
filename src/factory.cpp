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

namespace tt_solutions {

namespace impl {

FactoryBase::ClassMap *FactoryBase::sm_typeidx;
FactoryBase::ClassMap *FactoryBase::sm_nameidx;

void FactoryBase::Register()
{
    if (!sm_typeidx) {
        sm_typeidx = new ClassMap;
        sm_nameidx = new ClassMap;
    }

    FactoryBase*& p1 = (*sm_typeidx)[m_type];
    FactoryBase*& p2 = (*sm_nameidx)[m_name];

    wxASSERT(p1 == NULL);
    wxASSERT_MSG(p2 == NULL,
        _T("Factory::Impl defined with duplicate name string"));

    p1 = p2 = this;
}

void FactoryBase::Unregister()
{
    wxASSERT(sm_nameidx && sm_nameidx);

    int n1 = sm_typeidx->erase(m_type);
    int n2 = sm_nameidx->erase(m_name);
    wxASSERT(n1 == 1 && n2 == 1);

    if (sm_nameidx->size() == 0) {
        delete sm_typeidx;
        delete sm_nameidx;
        sm_typeidx = sm_nameidx = NULL;
    }
}

FactoryBase *FactoryBase::Get(const std::type_info& type)
{
    if (sm_typeidx != NULL) {
        wxString key = wxString::FromAscii(type.name());
        ClassMap::const_iterator it = sm_typeidx->find(key);

        if (it != sm_typeidx->end())
            return it->second;
    }

    return NULL;
}

FactoryBase *FactoryBase::Get(const wxString& name)
{
    if (sm_nameidx != NULL) {
        ClassMap::const_iterator it = sm_nameidx->find(name);

        if (it != sm_nameidx->end())
            return it->second;
    }

    return NULL;
}

} // namespace impl

} // namespace tt_solutions
