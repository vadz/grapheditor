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
    /**
     * @brief Base class defining untyped factory interface.
     *
     * This class is never used directly, it only extracts type-independent
     * code from Factory::Impl which is used by Factory class.
     *
     * It is also used to maintain a registry of existing factories and
     * provides access to it by name or type information object.
     */
    class FactoryBase
    {
    public:
        /// Override to create a new object.
        virtual wxObject *New() const = 0;

        /// Override to return the default factory object.
        virtual const wxObject *GetDefault() const = 0;

        /// Returns the name of this factory.
        wxString GetName() const { return m_name; }

        /**
         * @brief Get factory for creating objects of the given type.
         *
         * May return @c NULL.
         */
        static FactoryBase *Get(const std::type_info& type);

        /**
         * @brief Get factory for creating objects of the given class.
         *
         * May return @c NULL.
         */
        static FactoryBase *Get(const wxString& name);

    protected:
        /**
         * @brief Constructor specifies the type and name of the objects
         * created by this factory.
         *
         * Neither @a type nor @a name can be changed later.
         */
        FactoryBase(const std::type_info& type, const wxString& name)
          : m_type(wxString::FromAscii(type.name())), m_name(name) { }

        virtual ~FactoryBase() { }

        /**
         * @brief Register this factory so that Get() could find it.
         *
         * This should be called from the derived class ctor.
         */
        void Register();

        /**
         * @brief Unregister the factory to ensure that Get() doesn't return it.
         *
         * This should be called from the derived class dtor.
         */
        void Unregister();

    private:
        /// Unimplemented copy ctor.
        FactoryBase(const FactoryBase&) { }

#ifdef DOXYGEN
        /// Factories registry allowing fast access by string key.
        typedef std::unordered_map<wxString, FactoryBase *> ClassMap;
#else
        WX_DECLARE_STRING_HASH_MAP(FactoryBase*, ClassMap);
#endif

        /// Registry indexing factories by their type name from type info.
        static ClassMap *sm_typeidx;

        /// Registry indexing factories by their name as specified in ctor.
        static ClassMap *sm_nameidx;

        /// The type name of the objects created by this factory from type info.
        wxString m_type;

        /// The name of this factory as specified in ctor.
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
 * obtain using <code>factory.GetDefault()</code>. The <code>New()</code>
 * function creates new objects by copy constructing them from the default
 * object.
 */
template <class T> class Factory
{
private:
    /// Just a shorter name for impl::FactoryBase.
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

    /**
     * @brief Constructor, will create objects using the given factory
     * implementation.
     *
     * This is only used internally.
     */
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

    /**
     * @brief Implementation of untyped base factory interface for this factory.
     *
     * The base factory interface uses untyped @c wxObject, reimplement it
     * using the derived class @c T.
     */
    class Impl : public FactoryBase
    {
    public:
        /**
         * @brief Constructor for the factory with the given name.
         *
         * Factories are singleton objects and are only created internally to
         * ensure that we have a single factory per type. Attempts to create
         * more than one will result in assert failures.
         */
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

        /**
         * @brief Implement the base class pure virtual.
         *
         * Factory creates objects by cloning a default one which is exactly
         * what is done here.
         *
         * Notice that a covariant return type is used here.
         */
        T *New() const override {
            return new T(*GetDefault());
        }

        /**
         * @brief Implement the base class pure virtual.
         *
         * Notice that a covariant return type is used here.
         */
        const T *GetDefault() const override {
            if (!m_default)
                m_default = new T;
            return m_default;
        }

        /**
         * @brief Return the unique factory for this type.
         *
         * May return @c NULL if no factory for this type had been created yet.
         */
        static FactoryBase *Get() {
            return sm_this;
        }

    private:
        /**
         * @brief The default object.
         *
         * This object is created on demand by GetDefault(), don't access it
         * directly.
         */
        mutable T *m_default;

        /**
         * @brief The unique factory instance.
         *
         * May be @c NULL if the factory hadn't been created yet or was already
         * destroyed.
         */
        static FactoryBase *sm_this;
    };

private:
    /**
     * @brief Ensure that the factory creates objects of an appropriate type.
     *
     * Verify that the factory creates objects which are compatible (i.e. are
     * the same or derive from) our type T.
     *
     * If this is not the case, reset the factory pointer to @c NULL to avoid
     * using an incompatible factory implementation.
     */
    void CheckType() {
        if (m_impl && dynamic_cast<const T*>(m_impl->GetDefault()) == NULL)
            m_impl = NULL;
    }

    /**
     * @brief The real factory implementation.
     *
     * Only @c NULL if the factory was initialized improperly.
     */
    FactoryBase *m_impl;
};

template <class T> impl::FactoryBase *Factory<T>::Impl::sm_this;

} // namespace tt_solutions

#endif // FACTORY_H
