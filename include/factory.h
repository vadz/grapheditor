/////////////////////////////////////////////////////////////////////////////
// Name:        factory.h
// Purpose:     Object Factory
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
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

/**
 * @brief Object factory.
 *
 * To allow a class to be created by <code>Factory</code>, it is necessary
 * to define <code>Factory::Impl</code> for that class:
 * @code
 *  Factory<MyClass>::Impl myclass("myclass");
 * @endcode
 *
 * Instances of the class can then be created using the string name used
 * when defining the <code>Impl</code>, e.g.:
 * @code
 *  Factory<MyBase> factory(classname);
 *  MyBase *p = factory.New();
 * @endcode
 *
 * where <code>MyBase</code> is a base class for <code>MyClass</code> and
 * <code>classname</code> is the name "myclass".
 *
 * <code>Factory::Impl</code> doesn't need to be defined for
 * <code>MyBase</code>, just for the types that will actually be created.
 *
 * There are also constructors that will create a factory for the same type
 * as another existing object, or for a given typeid.
 *
 * The factory caches a default instance of the object, which you can
 * obtain using <code>factory.GetDefault()</code>. The <code>New</code>
 * function creates new objects by copy constructing them from the default
 * object.
 */
template <class T> class Factory
{
private:
    typedef impl::FactoryBase FactoryBase;

public:
    /**
     * @brief Default constructor, defaults to its own type.
     */
    Factory() : m_impl(Impl::Get()) { }

    /**
     * @brief Constructor, will create objects of the type given by the
     * <code>name</code>. 
     * 
     * The name is the name given when the <code>Impl</code> is defined.
     */
    Factory(const wxString& name)
      : m_impl(FactoryBase::Get(name)) {
        CheckType();
    }

    /**
     * @brief Constructor, will create objects of the type given by the
     * <code>type_info</code>.
     */
    Factory(const std::type_info& ti)
      : m_impl(FactoryBase::Get(ti)) {
        CheckType();
    }

    /**
     * @brief Constructor, will create objects of the same type as
     * <code>obj</code>.
     */
    Factory(const wxObject *obj)
      : m_impl(FactoryBase::Get(typeid(*obj))) {
        CheckType();
    }

    /**
     * @brief Constructor, will create objects of the same type as
     * <code>obj</code>.
     */
    Factory(const wxObject& obj)
      : m_impl(FactoryBase::Get(typeid(obj))) {
        CheckType();
    }

    Factory(FactoryBase *impl) : m_impl(impl) {
        wxASSERT(impl != NULL);
    }

    /**
     * @brief Conversion, makes the factory assignable to factories of
     * the base class.
     */
    template <class U> operator Factory<U>() const {
        U* u = (T*)NULL;
        (void)u;
        return Factory<U>(m_impl);
    }

    /**
     * @brief True if the factory could be created.
     */
    operator bool() const {
        return m_impl != NULL;
    }

    /**
     * @brief Create a new instance, uses the copy constructor to copy
     * the default object.
     */
    T *New() const {
        return static_cast<T*>(m_impl->New());
    }

    /**
     * @brief Returns the default object.
     */
    const T& GetDefault() const {
        return *static_cast<const T*>(m_impl->GetDefault());
    }

    /**
     * @brief Returns the name used to define the <code>Impl</code>.
     */
    wxString GetName() const {
        return m_impl->GetName();
    }

    class Impl : public FactoryBase
    {
    public:
        Impl(const wxString& name)
          : FactoryBase(typeid(T), name), m_default(NULL) {
            FactoryBase::Register();
            wxASSERT(sm_this == NULL);
            sm_this = this;
        }
        ~Impl() {
            FactoryBase::Unregister();
            delete m_default;
            sm_this = NULL;
        }

        T *New() const {
            return new T(*GetDefault());
        }

        const T *GetDefault() const {
            if (!m_default)
                m_default = new T;
            return m_default;
        }

        static FactoryBase *Get() {
            return sm_this;
        }

    private:
        mutable T *m_default;
        static FactoryBase *sm_this;
    };

private:
    void CheckType() {
        if (m_impl && dynamic_cast<const T*>(m_impl->GetDefault()) == NULL)
            m_impl = NULL;
    }

    FactoryBase *m_impl;
};

template <class T> impl::FactoryBase *Factory<T>::Impl::sm_this;

} // namespace tt_solutions

#endif // FACTORY_H
