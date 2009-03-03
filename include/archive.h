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
 *  Archive::const_iterator it, end;
 *  for (tie(it, end) = archive.GetItems(); it != end; ++it) {
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
 * To store an object's member that is itself an object, store the inner
 * object as normal, then store its id as the outer object's member.
 *
 * If you need to deal with graphs of objects, perhaps with loops, then
 * when deserialising you can associate an instance with an <code>Item</code>,
 * using <code>SetInstance()</code>, so that you know not to create
 * a second instance.
 *
 * <code>Archive</code> doesn't deal with object creation when deserialising.
 * If you need a generic way to do that then
 * <code>wxClassInfo::CreateObject()</code> or <code>Factory</code> can be
 * used.
 *
 * To extend the types supported by <code>Item::Get</code> and
 * <code>Item::Put</code>, implement an insertor/extractor pair, e.g.:
 * @code
 *  bool Insert(Archive::Item& arc, const wxString& name, const wxFont& value);
 *  bool Extract(const Archive::Item& arc, const wxString& name, wxFont& value);
 * @endcode
 * These should be added to the namespace of the type being stored.
 */
class Archive
{
public:
    /**
     * @brief Serialisation archive item, holds an object in the archive.
     */
    class Item
    {
    private:
        typedef std::map<wxString, wxString> StringMap;

        Item(Archive& archive,
             const wxString& name,
             const wxString& id,
             const wxString& sort);

        ~Item() { SetInstance(NULL); }

    public:
        //@{
        /** * @brief The class of the object, alphanumerics only.  */
        void SetClass(const wxString& name) { m_class = name; }
        wxString GetClass() const { return m_class; }
        //@}

        /** @brief Returns the item's id. */
        wxString GetId() const { return m_id; }
        /** @brief Returns the item's sort key. */
        wxString GetSort() const { return m_sort; }

        //@{
        /**
         * @brief Used to associate a deserialised object with the
         * <code>Item</code> that stored it.
         *
         * When deserialising an object that is referred to by several other
         * objects, a pointer can be kept to the first instance deserialised,
         * so that it can also be used by later objects that need it.
         */
        void SetInstance(wxObject *instance, bool owns = false);
        wxObject *GetInstance() const { return m_instance; }
        template <class T> T* GetInstance() const {
            return dynamic_cast<T*>(m_instance);
        }
        //@}

        /**
         * @brief Returns the <code>Archive</code> object holding this item.
         * */
        Archive& GetArchive() const { return m_archive; }

        /** @brief Equivalent to <code>GetArchive()->IsStoring()</code>. */
        bool IsStoring() const { return m_archive.IsStoring(); }
        /** @brief Equivalent to <code>GetArchive()->IsExtracting()</code>. */
        bool IsExtracting() const { return m_archive.IsExtracting(); }

        //@{
        /**
         * @brief Stores an attribute.
         *
         * @returns true on success, or false if <code>name</code> is not
         * unique within this item.
         *
         * The types supported can be extended by implementing an
         * insertor/extractor pair for new types.
         */
        bool Put(const wxString& name, const wxString& value = wxEmptyString);
        bool Put(const wxString& name, const wxChar *value);

        template <class T> bool Put(const wxString& name, const T& value) {
            return Insert(*this, name, value);
        }
        //@}

        //@{
        /**
         * @brief Get an attribute.
         *
         * @returns On success assigns to <code>value</code> and returns true.
         * On failure returns false and leaves <code>value</code> unchanged.
         *
         * The types supported can be extended by implementing an
         * insertor/extractor pair for new types.
         */
        bool Get(const wxString& name, wxString& value) const;
        wxString Get(const wxString& name) const;

        template <class T> bool Get(const wxString& name, T& value) const {
            return Extract(*this, name, value);
        }

        template <class T> T Get(const wxString& name) const {
            T t;
            Get(name, t);
            return t;
        }
        //@}

        /** @brief Returns true if an attribute exists with the given name. */
        bool Has(const wxString& name) const;
        /** @brief Removes an attribute. */
        bool Remove(const wxString& name);

        //@{
        /**
         * @brief Puts an attribute if <code>IsStoring()</code> or
         * gets it otherwise.
         *
         * @param name The name of the attribute.
         * @param value A variable that is either read or written to.
         * @param def Ignored when extracting. When storing, if the value
         * compares equal to def then it is not stored.
         *
         * Note that <code>def</code> does not provide a default for loading.
         */
        template <class T>
        void Exch(const wxString& name, T& value, const T& def = T()) {
            if (m_archive.IsExtracting())
                Get(name, value);
            else if (value != def)
                Put(name, value);
        }
        //@}

        typedef StringMap::iterator iterator;
        typedef StringMap::const_iterator const_iterator;

        typedef std::pair<iterator, iterator> iterator_pair;
        typedef std::pair<const_iterator, const_iterator> const_iterator_pair;

        //@{
        /**
         * @brief Get an iterator pair returning all the attributes.
         */
        iterator_pair GetAttribs();
        const_iterator_pair GetAttribs() const;
        //@}

    private:
        Archive& m_archive;
        wxString m_class;
        wxString m_id;
        wxString m_sort;
        StringMap m_attribs;
        wxObject *m_instance;
        bool m_owns;

        friend class Archive;
    };

private:
    typedef std::map<wxString, Item*> ItemMap;
    typedef std::multimap<wxString, Item*> SortMap;

public:
    Archive();
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
     * True by default, but set false by <code>Load</code>. Used by
     * <code>Item::Exch</code>.
     */
    void SetStoring(bool storing = true) { m_storing = storing; }
    bool IsStoring() const { return m_storing; }
    //@}

    /** @brief Same as !IsStoring(). */
    bool IsExtracting() const { return !m_storing; }

    /**
     * @brief Add an <code>Item</code> to the archive.
     *
     * @param classname A name for the class, alphanumerics only.
     * @param id Must be unique, ASCII only.
     * @param sortkey Optional, ASCII only.
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
    /** @brief Equivalent to Get(id)->GetInstance(id). */
    wxObject *GetInstance(const wxString& id) const;
    template <class T> T* GetInstance(const wxString& id) const {
        return dynamic_cast<T*>(GetInstance(id));
    }
    //@}

    /**
     * @brief Makes an id for an object using its memory address.
     *
     * Using this function to generate an id for an item is optional.
     * The id can in fact be any unique ASCII string.
     *
     * This function generates an id or an object by simply using the address
     * it has when it is being serialised. It is unique and
     * two references to the same object will get the same id.
     *
     * For reference counted objects, you can use the address of the
     * representation object, e.g. for a wxBitmap you could use
     * <code>wxString id = MakeId(bmp.GetRefData())</code>
     */
    static wxString MakeId(const void *p);

    /**
     * @brief Sets an item's sort key.
     *
     * You can set an items sort key when you add it with <code>Put</code>
     * or later with this function.
     */
    void SortItem(Item& item, const wxString& key);

    typedef SortMap::iterator iterator;
    typedef SortMap::const_iterator const_iterator;

    typedef std::pair<iterator, iterator> iterator_pair;
    typedef std::pair<const_iterator, const_iterator> const_iterator_pair;

    //@{
    /**
     * @brief Returns an iterator range covering some or all of the
     * <code>Item</code> objects in the archive.
     *
     * If <code>prefix</code> is provided then returns just the items whose
     * sort key begins with that prefix. Or if the <code>prefix</code> is
     * omitted, returns all items.
     * @code
     *  Archive::const_iterator it, end;
     *  for (tie(it, end) = archive.GetItems(prefix); it != end; ++it)
     *      ...
     * @endcode
     *
     * The items are returned in the order of their sort keys.
     */
    iterator_pair GetItems(const wxString& prefix = wxEmptyString);
    const_iterator_pair GetItems(const wxString& prefix = wxEmptyString) const;
    //@}

private:
    void Sort() const;
    void SortAdd(Item *item) const;
    void SortRemove(Item *item) const;
    iterator_pair DoGetItems(const wxString& prefix) const;

    ItemMap m_items;
    mutable SortMap m_sort;
    bool m_storing;
};

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
    struct CStr {
        const wxChar *operator()(const wxString& str) { return str; }
    } c_str;

    wxString str;
    if (!arc.Get(name, str))
        return false;

    std::basic_istringstream<wxChar> ss(c_str(str));
    T val;
    ss >> val;

    if (ss)
        value = val;

    return ss != NULL;
}

} // namespace tt_solutions

#endif // ARCHIVE_H
