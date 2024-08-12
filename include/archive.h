/////////////////////////////////////////////////////////////////////////////
// Name:        archive.h
// Purpose:     Serialisation archive
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <wx/wx.h>

#include <sstream>
#include <map>

/**
 * @file archive.h
 * @brief Serialisation archive.
 */

namespace tt_solutions {

inline bool ShouldInsert(const wxFont& value, const wxFont& def)
{
    return !value.IsSameAs(def);
}

inline bool ShouldInsert(const wxIcon& value, const wxIcon& def)
{
    return !value.IsSameAs(def);
}

inline bool ShouldInsert(const wxBitmap& value, const wxBitmap& def)
{
    return !value.IsSameAs(def);
}

inline bool ShouldInsert(const wxImage& value, const wxImage& def)
{
    return !value.IsSameAs(def);
}

/**
 * @brief Compare function for @c Archive::Item::Exch.
 *
 * @c %ShouldInsert() is called by @c Archive::Item::Exch() when storing a
 * value, the value is only stored if it returns true.
 *
 * The default implementation returns true if the @c value parameter compares
 * unequal to the @c def parameter using the operator @c !=. This gives @c
 * @c Exch() its usual behaviour of only storing attributes that are different
 * to their default values.
 *
 * You would not normally call this function directly, however you might
 * implement an overload if you were adding @c Insert() and @c Extract()
 * overloads to handle a new type that cannot be tested for inequality with
 * the @c != operator.
 */
template <class T> bool ShouldInsert(const T& value, const T& def)
{
    return value != def;
}

/**
 * @brief Serialisation archive.
 *
 * To store, an <code>Item</code> is added to the archive
 * for each object that is to be stored:
 * @code
 *  Archive archive;
 *  Archive::Item *arc = archive.Put("myclass", Archive::MakeId(obj));
 *  arc->Put("text", obj->GetText());
 *  arc->Put("size", obj->GetSize());
 *  // store more objects...
 *  archive.Save(stream);
 * @endcode
 *
 * Each <code>Item</code> has a unique id and optionally a non-unique sort
 * key. When deserialising:
 * @code
 *  Archive archive;
 *  archive.Load(stream);
 * @endcode
 *
 * items can be fetched directly using the id:
 * @code
 * Archive::Item *arc = archive.Get(id);
 * @endcode
 *
 * if it is known. Alternatively you can iterate over the <code>Item</code>
 * objects, in which case you see the items in the order given by the sort
 * key:
 * @code
 *  for (const auto it : MakeRange(archive.GetItems())) {
 *      const Archive::Item *arc = it->second;
 *      if (arc->GetClass() == "myclass") {
 *          wxString text = arc->Get("text");
 *          wxSize size = arc->Get<wxSize>("size");
 *          MyClass *obj = new MyClass(text, size);
 *          // do something with extracted obj
 *      }
 *  }
 * @endcode
 *
 * <code>Archive</code> doesn't deal with object creation when deserialising.
 * If you need a generic way to do that then
 * <code>wxClassInfo::CreateObject()</code> or <code>Factory</code> can be
 * used.
 *
 * The types supported can be extended by adding new overloads of @c Insert()
 * and @c Extract().
 *
 * @see Archive::Item
 */
class Archive
{
public:
    /**
     * @brief Serialisation archive item, holds an object in the archive.
     *
     * An @c Archive holds a collection of these @c Item objects, with each
     * one representing an object. The @c Item contains a collection of
     * key/value pairs representing the serialised object's attributes.
     *
     * Creation of @c Item objects is done using @c Archive::Put, then
     * the object's attributes are stored using @c Archive::Item::Put, for
     * example:
     * @code
     *  Archive archive;
     *  Archive::Item *arc = archive.Put("myclass", Archive::MakeId(obj));
     *  arc->Put("text", obj->GetText());
     *  arc->Put("size", obj->GetSize());
     * @endcode
     *
     * @see Archive
     */
    class Item
    {
    private:
        /**
         * @brief Map used to store item attributes.
         *
         * A map with string keys and string values used for storing the items
         * attributes.
         */
        typedef std::map<wxString, wxString> StringMap;

        /**
         * @brief Item constructor.
         *
         * This is used by Archive::Put() to add items to the archive.
         */
        Item(Archive& archive,
             const wxString& name,
             const wxString& id,
             const wxString& sort);

        Item(const Item&) = delete;
        Item(Item&&) = delete;
        Item& operator=(const Item&) = delete;
        Item& operator=(Item&&) = delete;

        ~Item() { SetInstance(NULL); }

    public:
        //@{
        /** @brief The class of the object, alphanumerics only. */
        void SetClass(const wxString& name) { m_class = name; }
        wxString GetClass() const { return m_class; }
        //@}

        /**
         * @brief Returns the item's id.
         *
         * Each @c Item has a unique id. It is assigned when the @c Item is
         * created with @c Archive::Put and can't be changed. It can be
         * used when deserialising to fetch @c Items randomly.
         *
         * @see Archive::MakeId()
         */
        wxString GetId() const { return m_id; }
        /**
         * @brief Returns the item's sort key.
         *
         * The sort key determines the order the items are returned by
         * @c Archive::GetItems. It can be set when the @c Item is created
         * with @c Archive::Put or set latter with @c Archive::SortItem.
         */
        wxString GetSort() const { return m_sort; }

        //@{
        /**
         * @brief Used to associate a deserialised object with the
         * <code>Item</code> that stored it.
         *
         * When storing a collection of objects that form a graph with loops,
         * you will encounter the same object more than once during
         * serialisation or extraction. You can avoid storing the same object
         * more than once by using an id that will always be the same for the
         * same instance, see @c Archive::MakeId().
         *
         * Similarly when deserialising, you can avoid deserialising the same
         * object more than once by keeping a reference to the first instance
         * you extract using @c Archive::Item::SetInstance(). The same
         * instance can be be returned the next time the same object is
         * encountered during deserialisation.
         */
        void SetInstance(wxObject *instance, bool owns = false);
        wxObject *GetInstance() const { return m_instance; }
        template <class T> T* GetInstance() const {
            return dynamic_cast<T*>(m_instance);
        }
        //@}

        /**
         * @brief Returns the <code>Archive</code> object holding this item.
         */
        Archive& GetArchive() const { return m_archive; }

        /**
         * @brief Equivalent to <code>@link tt_solutions::Archive::IsStoring
         * GetArchive()->IsStoring()@endlink</code>.
         *
         * @see Exch()
         */
        bool IsStoring() const { return m_archive.IsStoring(); }
        /**
         * @brief Equivalent to <code>!@link tt_solutions::Archive::IsStoring
         * GetArchive()->IsStoring()@endlink</code>.
         *
         * @see Exch()
         */
        bool IsExtracting() const { return m_archive.IsExtracting(); }

        /**
         * @brief Stores an attribute.
         *
         * The attributes are stored as @c wxString values, if @b T is any
         * other type then @c %Put() delegates to @c Insert() to convert the
         * type to a @c wxString value that can be stored. See the overloads
         * declared in the header to see what types are supported. Support for
         * additional types can be added by implementing additional overloads
         * of @c Insert().
         *
         * @param name Name of the attribute to set.
         * @param value The value to store.
         *
         * @returns true on success, or false if <code>name</code> is not
         * unique within this item.
         *
         * @see Insert()
         */
        template <class T>
        bool Put(const wxString& name, const T& value) {
            return Insert(*this, name, value);
        }
        /**
         * @brief Stores an attribute.
         *
         * The attributes are stored as @c wxString values, if @b T is any
         * other type then @c %Put() delegates to @c Insert() to convert the
         * type to a @c wxString value that can be stored. See the overloads
         * declared in the header to see what types are supported. Support for
         * additional types can be added by implementing additional overloads
         * of @c Insert().
         *
         * @param name Name of the attribute to set.
         * @param value The value to store.
         * @param param Optional extra parameter passed to @c Insert.
         *
         * @returns true on success, or false if <code>name</code> is not
         * unique within this item.
         *
         * @see Insert()
         */
        template <class T, class U>
        bool Put(const wxString& name, const T& value, const U& param) {
            return Insert(*this, name, value, param);
        }
        /** @cond */
        bool Put(const wxString& name, const wxString& value = wxEmptyString);
        bool Put(const wxString& name, const wxChar *value);
        /** @endcond */

        /**
         * @brief Get an attribute.
         *
         * The attributes are stored as @c wxString values, if @b T is any
         * other type then @c %Get() delegates to @c Extract() to convert from
         * the @c wxString to a @b T. See the overloads declared in archive.h
         * to see what types are supported. Support for additional types can
         * be added by implementing additional overloads of @c Extract().
         *
         * @param name Name of the attribute to get.
         *
         * @returns On success returns the attribute value, on failure returns
         * @c T().
         */
        template <class T> T Get(const wxString& name) const {
            T t;
            Get(name, t);
            return t;
        }
        /**
         * @brief Get an attribute.
         *
         * The attributes are stored as @c wxString values, if @b T is any
         * other type then @c %Get() delegates to @c Extract() to convert from
         * the @c wxString to a @b T. See the overloads declared in archive.h
         * to see what types are supported. Support for additional types can
         * be added by implementing additional overloads of @c Extract().
         *
         * @param name Name of the attribute to get.
         * @param value Reference to variable to receive the value.
         *
         * @returns On success assigns to <code>value</code> and returns @c
         * true. On failure returns @c false and leaves <code>value</code>
         * unchanged.
         */
        template <class T> bool Get(const wxString& name, T& value) const {
            return Extract(*this, name, value);
        }
        /**
         * @brief Get an attribute.
         *
         * The attributes are stored as @c wxString values, if @b T is any
         * other type then @c %Get() delegates to @c Extract() to convert from
         * the @c wxString to a @b T. See the overloads declared in archive.h
         * to see what types are supported. Support for additional types can
         * be added by implementing additional overloads of @c Extract().
         *
         * @param name Name of the attribute to get.
         * @param value Reference to variable to receive the value.
         * @param param Optional extra parameter passed to @c Extract().
         *
         * @returns On success assigns to <code>value</code> and returns @c
         * true. On failure returns @c false and leaves <code>value</code>
         * unchanged.
         */
        template <class T, class U>
        bool Get(const wxString& name, T& value, const U& param) const {
            return Extract(*this, name, value, param);
        }
        /** @cond */
        wxString Get(const wxString& name) const;
        bool Get(const wxString& name, wxString& value) const;
        /** @endcond */

        /** @brief Returns true if an attribute exists with the given name. */
        bool Has(const wxString& name) const;
        /** @brief Removes an attribute. */
        bool Remove(const wxString& name);

        /**
         * @brief @c Put() an attribute if @c IsStoring() is @c true or @c
         * Get() it otherwise.
         *
         * @c %Exch() can be used to write a single function that takes care
         * of both serialising and deserialising the members of an object.
         * When storing, only stores the attribute if it is different to the
         * default value given by @c def.
         *
         * @param name The name of the attribute.
         * @param value A variable that is either read or written to.
         * @param def Ignored when extracting. When storing, if the value
         * compares equal to def then it is not stored.
         *
         * @see Put() @n Get() @n ShouldInsert()
         */
        template <class T>
        void Exch(const wxString& name, T& value, const T& def = T()) {
            if (m_archive.IsExtracting())
                Get(name, value);
            else if (ShouldInsert(value, def))
                Put(name, value);
        }
        /**
         * @brief Puts an attribute if @c IsStoring() is @c true or gets it
         * otherwise.
         *
         * @c %Exch() can be used to write a single function that takes care
         * of both serialising and deserialising the members of an object.
         * When storing, only stores the attribute if it is different to the
         * default value given by @c def.
         *
         * @param name The name of the attribute.
         * @param value A variable that is either read or written to.
         * @param def Ignored when extracting. When storing, if the value
         * compares equal to def then it is not stored.
         * @param param Optional extra parameter passed to @c Insert() and @c
         * Extract().
         *
         * @see Put() @n Get() @n ShouldInsert()
         */
        template <class T, class U>
        void Exch(const wxString& name, T& value, const T& def, const U& param) {
            if (m_archive.IsExtracting())
                Get(name, value, param);
            else if (ShouldInsert(value, def))
                Put(name, value, param);
        }

        //@{
        /** @brief An iterator type for returning the Item's attributes. */
        typedef StringMap::iterator iterator;
        typedef StringMap::const_iterator const_iterator;
        //@}

        //@{
        /** @brief A begin/end pair of iterators. */
        typedef std::pair<iterator, iterator> iterator_pair;
        typedef std::pair<const_iterator, const_iterator> const_iterator_pair;
        //@}

        //@{
        /** @brief Get an iterator pair returning all the attributes. */
        iterator_pair GetAttribs();
        const_iterator_pair GetAttribs() const;
        //@}

    private:
        Archive& m_archive;     ///< Back pointer to the archive.
        wxString m_class;       ///< Class of the item.
        wxString m_id;          ///< Unique id of the item.
        wxString m_sort;        ///< Optional sort order.
        StringMap m_attribs;    ///< Items attributes map.

        /// Optional pointer to another instance of the same item.
        wxObject *m_instance;

        /// True if we own, i.e. should delete, m_instance pointer.
        bool m_owns;

        friend class Archive;
    };

private:
    /**
     * @brief Map used to store the archive items.
     *
     * Map with string keys and items as values.
     */
    typedef std::map<wxString, Item*> ItemMap;

    /**
     * @brief Map used to store sort order of the items.
     *
     * Notice that this map is always used, even if no explicit sort order is
     * set.
     */
    typedef std::multimap<wxString, Item*> SortMap;

public:
    Archive();
    Archive(const Archive&) = delete;
    Archive(Archive&&) = delete;
    Archive& operator=(const Archive&) = delete;
    Archive& operator=(Archive&&) = delete;
    ~Archive();

    /** @brief Deletes all the <code>Item</code> objects in the archive. */
    void Clear();

    /** @brief Load a previously saved archive from a stream.  */
    bool Load(wxInputStream& stream);
    /** @brief Save the archive to a stream. */
    bool Save(wxOutputStream& stream) const;

    //@{
    /**
     * @brief The 'storing' flag.
     *
     * True by default, but set false by <code>Load()</code>. Used by
     * <code>Item::Exch()</code>.
     */
    void SetStoring(bool storing = true) { m_storing = storing; }
    bool IsStoring() const { return m_storing; }
    //@}

    /** @brief Same as !IsStoring(). */
    bool IsExtracting() const { return !m_storing; }

    /**
     * @brief Add an <code>Item</code> to the archive.
     *
     * For each object you wish to store, you @c %Put() an @c Item, then
     * store the object's attributes into the @c Item with @c Item::Put().
     *
     * @param classname A name for the class, alphanumerics only.
     * @param id Must be unique, ASCII only. Can be used to retrieve the @c
     * Item randomly with @c Get().
     * @param sortkey Optional, ASCII only. Determines the order the items are
     * returned by @c GetItems().
     *
     * @returns a pointer to the newly created <code>Item</code>, or NULL
     * if the id already exists.
     */
    Item *Put(const wxString& classname,
              const wxString& id,
              const wxString& sortkey = wxEmptyString);
    /** @brief Delete an <code>Item</code> in the archive. */
    bool Remove(const wxString& id);

    //@{
    /**
     * @brief Returns an <code>Item</code> in the archive or NULL if the
     * id does not exist.
     */
    Item *Get(const wxString& id);
    const Item *Get(const wxString& id) const;
    //@}

    //@{
    /**
     * @brief Equivalent to <code>@link
     * tt_solutions::Archive::Item::GetInstance
     * Get(id)->GetInstance()@endlink</code>.
     */
    wxObject *GetInstance(const wxString& id) const;
    template <class T> T* GetInstance(const wxString& id) const {
        return dynamic_cast<T*>(GetInstance(id));
    }
    //@}

    /**
     * @brief Makes an id for an object using its memory address.
     *
     * This function generates an id or an object by simply using the address
     * it has when it is being serialised. It is unique and two references to
     * the same object will get the same id.
     *
     * When storing a graph of objects containing loops, you will encounter
     * the same instance of some objects more than once during serialisation.
     * Using the address of the object gives an easy way to check whether the
     * instance has already been stored.
     *
     * For reference counted objects, you can use the address of the
     * representation object, e.g. if you look at the @c wxBitmap
     * implementation of @c Insert() in archive.cpp you will see that it uses:
     * <code>wxString id = Archive::MakeId(value.GetRefData());</code>
     */
    static wxString MakeId(const void *p);

    /**
     * @brief Sets an item's sort key.
     *
     * You can set an items sort key when you add it with <code>Put()</code>
     * or later with this function.
     *
     * The sort key determines the order the items are returned by @c
     * GetItems().
     */
    void SortItem(Item& item, const wxString& key);

    //@{
    /** @brief An iterator type for returning the @c Archive's @c Items. */
    typedef SortMap::iterator iterator;
    typedef SortMap::const_iterator const_iterator;
    //@}

    //@{
    /** @brief A begin/end pair of iterators. */
    typedef std::pair<iterator, iterator> iterator_pair;
    typedef std::pair<const_iterator, const_iterator> const_iterator_pair;
    //@}

    //@{
    /**
     * @brief Returns an iterator range covering some or all of the
     * <code>Item</code> objects in the archive.
     *
     * If <code>prefix</code> is provided then returns just the items whose
     * sort key begins with that prefix. Or if the <code>prefix</code> is
     * omitted, returns all items.
     * @code
     *  for (const auto& it : MakeRange(archive.GetItems(prefix))) {
     *      ...
     *  }
     * @endcode
     *
     * The items are returned in the order of their sort keys.
     */
    iterator_pair GetItems(const wxString& prefix = wxEmptyString);
    const_iterator_pair GetItems(const wxString& prefix = wxEmptyString) const;
    //@}

private:
    /**
     * Ensure that all items are in sorted order in m_sort.
     *
     * Does nothing if the items are already sorted or calls SortAdd() for all
     * of them if they are not.
     */
    void Sort() const;

    /**
     * Adds an item to m_sort.
     *
     * Simply inserts the item with its sort key into m_sort map.
     */
    void SortAdd(Item *item) const;

    /**
     * Removes an item from m_sort.
     *
     * Removes the item from m_sort map. Notice that this is a relatively
     * expensive operation as the entire range of the items with the same sort
     * key needs to be checked.
     */
    void SortRemove(Item *item) const;

    /**
     * Implementation of the public GetItems().
     */
    iterator_pair DoGetItems(const wxString& prefix) const;

    /// The map containing all the items.
    ItemMap m_items;

    /**
     * Map containing items in their sort order.
     *
     * This map may be empty, Sort() must be called before accessing it to
     * ensure that it is initialized.
     */
    mutable SortMap m_sort;

    /**
     * True if we're storing the items or false if we're extracting them.
     */
    bool m_storing;
};

/** @cond */

bool Insert(Archive::Item& arc, const wxString& name, const wxPoint& value);
bool Extract(const Archive::Item& arc, const wxString& name, wxPoint& value);

bool Insert(Archive::Item& arc, const wxString& name, const wxSize& value);
bool Extract(const Archive::Item& arc, const wxString& name, wxSize& value);

bool Insert(Archive::Item& arc, const wxString& name, const wxRect& value);
bool Extract(const Archive::Item& arc, const wxString& name, wxRect& value);

bool Insert(Archive::Item& arc, const wxString& name, const wxColour& value);
bool Extract(const Archive::Item& arc, const wxString& name, wxColour& value);

bool Insert(Archive::Item& arc, const wxString& name, const wxFont& value);
bool Extract(const Archive::Item& arc, const wxString& name, wxFont& value);

bool Insert(Archive::Item& arc,
            const wxString& name,
            const wxIcon& value,
            wxBitmapType type = wxBITMAP_TYPE_PNG);

bool Extract(const Archive::Item& arc,
             const wxString& name,
             wxIcon& value,
             wxBitmapType type = wxBITMAP_TYPE_ANY);

bool Insert(Archive::Item& arc,
            const wxString& name,
            const wxBitmap& value,
            wxBitmapType type = wxBITMAP_TYPE_PNG);

bool Extract(const Archive::Item& arc,
             const wxString& name,
             wxBitmap& value,
             wxBitmapType type = wxBITMAP_TYPE_ANY);

bool Insert(Archive::Item& arc,
            const wxString& name,
            const wxImage& value,
            wxBitmapType type = wxBITMAP_TYPE_PNG);

bool Extract(const Archive::Item& arc,
             const wxString& name,
             wxImage& value,
             wxBitmapType type = wxBITMAP_TYPE_ANY);

inline const wxChar *c_str(const wxString& str) { return str; }

/** @endcond */

//@{
/**
 * @brief @c Insert and @c Extract store and load attributes to and from an
 * @c Archive::Item.
 *
 * @c %Insert() and @c %Extract() aren't used directly, instead @c
 * Archive::Item::Put() and @c Archive::Item::Get() are used to store and load
 * attributes. See also @c Archive::Item::Exch().
 *
 * Implementing new overloads of @c %Insert() and @c %Extract() has the effect
 * of extending @c Put and @c Get to be able to handle a new type, for
 * example:
 *
 * @code
 * bool Insert(Archive::Item& arc, const wxString& name, const MyType& value);
 * bool Extract(const Archive::Item& arc, const wxString& name, MyType& value);
 * @endcode
 *
 * The overload would typically convert between the new type and @c wxString
 * then store the string using the @c wxString overloads of @c
 * Archive::Item::Put and @c Archive::Item::Get().
 *
 * If the @c MyType value is itself an object that can't reasonably be
 * converted to a simple string, then it can be stored as a separate object
 * using @c Archive::Put(), and then its id stored as the @c wxString value of
 * the original attribute.
 *
 * When storing a collection of objects that form a graph with loops, you will
 * encounter the same object more than once during serialisation or
 * extraction. You can avoid storing the same object more than once by using
 * an id that will always be the same for the same instance, see @c
 * Archive::MakeId().
 *
 * Similarly when deserialising, you can avoid deserialising the same object
 * more than once by keeping a reference to the first instance you extract
 * using @c Archive::Item::SetInstance() and @c Archive::Item::GetInstance().
 *
 * Your overloads can have an extra parameter of any type. This extra
 * parameter can then be passed to @c Put(), @c Get() or @c Exch() and it will
 * be used in the calls to your @c %Insert and @c %Extract functions.
 *
 * Your new inserter and extractor should be added to the same namespace as @c
 * MyType rather than to @c tt_solutions, they will be found by Koenig lookup.
 *
 * To see the types already supported, see the overloads already defined in
 * archive.h.
 *
 * @see
 *  Archive::Item::Get() @n
 *  Archive::Item::Put() @n
 *  Archive::Item::Exch()
 */
template <class T>
bool Insert(Archive::Item& arc, const wxString& name, const T& value)
{
    std::basic_ostringstream<wxChar> ss;
    ss << value;
    return arc.Put(name, wxString(ss.str()));
}

template <class T>
bool Extract(const Archive::Item& arc, const wxString& name, T& value)
{
    wxString str;
    if (!arc.Get(name, str))
        return false;

    std::basic_istringstream<wxChar> ss(c_str(str));
    T val;
    ss >> val;

    if (ss)
        value = val;

    return ss.good();
}
//@}

} // namespace tt_solutions

#endif // ARCHIVE_H
