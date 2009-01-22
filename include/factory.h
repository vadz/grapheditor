/////////////////////////////////////////////////////////////////////////////
// Name:        factory.h
// Purpose:     Object Factory
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef FACTORY_H
#define FACTORY_H

#include <wx/wx.h>

#include <typeinfo>

/**
 * @file factory.h
 * @brief Object factory.
 */

namespace tt_solutions {

/*
 * Implementation classes
 */
namespace impl
{
    class FactoryBase
    {
    public:
        virtual wxObject *New() const = 0;
        virtual const wxObject *GetDefault() const = 0;

        wxString GetName() const { return m_name; }

        static FactoryBase *Get(const std::type_info& type);
        static FactoryBase *Get(const wxString& name);

    protected:
        FactoryBase(const std::type_info& type, const wxString& name)
          : m_type(wxString::FromAscii(type.name())), m_name(name) { }

        virtual ~FactoryBase() { }

        void Register();
        void Unregister();

    private:
        FactoryBase(const FactoryBase&) { }

        WX_DECLARE_STRING_HASH_MAP(FactoryBase*, ClassMap);
        static ClassMap *sm_typeidx;
        static ClassMap *sm_nameidx;

        wxString m_type;
        wxString m_name;
    };

} // namespace impl

template <class T> class Factory
{
public:
    Factory() : m_impl(&sm_impl) { }

    Factory(const wxString& name) : m_impl(impl::FactoryBase::Get(name)) {
        CheckType();
    }
    Factory(const std::type_info& ti) : m_impl(impl::FactoryBase::Get(ti)) {
        CheckType();
    }
    Factory(impl::FactoryBase *impl) : m_impl(impl) {
        wxASSERT(impl != NULL);
    }

    template <class U> operator Factory<U>() const {
        U* u = (T*)NULL;
        (void)u;
        return Factory<U>(m_impl);
    }

    operator bool() const {
        return m_impl != NULL;
    }

    T *New() const {
        return static_cast<T*>(m_impl->New());
    }
    const T& GetDefault() const {
        return *static_cast<const T*>(m_impl->GetDefault());
    }

    wxString GetName() const {
        return m_impl->GetName();
    }

private:
    class Impl : public impl::FactoryBase
    {
    public:
        Impl(const wxString& name)
          : impl::FactoryBase(typeid(T), name), m_default(NULL) {
            impl::FactoryBase::Register();
        }
        ~Impl() {
            impl::FactoryBase::Unregister();
            delete m_default;
        }

        T *New() const {
            return new T(*GetDefault());
        }

        const T *GetDefault() const {
            if (!m_default)
                m_default = new T;
            return m_default;
        }

    private:
        mutable T *m_default;
    };

    void CheckType() {
        if (m_impl && dynamic_cast<const T*>(m_impl->GetDefault()) == NULL)
            m_impl = NULL;
    }

    impl::FactoryBase *m_impl;
    static Impl sm_impl;
};

} // namespace tt_solutions

#endif // FACTORY_H
