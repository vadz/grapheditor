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

class Archive
{
public:
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
        void SetClass(const wxString& name) { m_class = name; }
        wxString GetClass() const { return m_class; }

        wxString GetId() const { return m_id; }
        wxString GetSort() const { return m_sort; }

        void SetInstance(wxObject *instance, bool owns = false);
        wxObject *GetInstance() const { return m_instance; }

        template <class T> T* GetInstance() const {
            return dynamic_cast<T*>(m_instance);
        }

        Archive& GetArchive() const { return m_archive; }

        bool IsStoring() const { return m_archive.IsStoring(); }
        bool IsExtracting() const { return m_archive.IsExtracting(); }

        bool Put(const wxString& name, const wxString& value = wxEmptyString);
        bool Put(const wxString& name, const wxChar *value);
        bool Get(const wxString& name, wxString& value) const;
        wxString Get(const wxString& name) const;
        bool Has(const wxString& name) const;
        bool Remove(const wxString& name);

        template <class T> bool Put(const wxString& name, const T& value) {
            return Insert(*this, name, value);
        }
        template <class T> bool Get(const wxString& name, T& value) const {
            return Extract(*this, name, value);
        }

        template <class T> T Get(const wxString& name) const {
            T t;
            Get(name, t);
            return t;
        }

        template <class T>
        void Exch(const wxString& name, T& value, const T& def = T()) {
            if (m_archive.IsExtracting())
                Get(name, value);
            else if (value != def)
                Put(name, value);
        }

        typedef StringMap::iterator iterator;
        typedef StringMap::const_iterator const_iterator;

        typedef std::pair<iterator, iterator> iterator_pair;
        typedef std::pair<const_iterator, const_iterator> const_iterator_pair;

        iterator_pair GetAttribs();
        const_iterator_pair GetAttribs() const;

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

    void Clear();

    bool Load(wxInputStream& stream);
    bool Save(wxOutputStream& stream) const;

    void SetStoring(bool storing = true) { m_storing = storing; }
    bool IsStoring() const { return m_storing; }
    bool IsExtracting() const { return !m_storing; }

    Item *Put(const wxString& classname,
              const wxString& id,
              const wxString& sortkey = wxEmptyString);
    bool Remove(const wxString& id);

    Item *Get(const wxString& id);
    const Item *Get(const wxString& id) const;

    wxObject *GetInstance(const wxString& id) const;
    template <class T> T* GetInstance(const wxString& id) const {
        return dynamic_cast<T*>(GetInstance(id));
    }
    static wxString MakeId(const void *p);

    void SortItem(Item& item, const wxString& key);

    typedef SortMap::iterator iterator;
    typedef SortMap::const_iterator const_iterator;

    typedef std::pair<iterator, iterator> iterator_pair;
    typedef std::pair<const_iterator, const_iterator> const_iterator_pair;

    iterator_pair GetItems(const wxString& first = wxEmptyString);
    const_iterator_pair GetItems(const wxString& first = wxEmptyString) const;

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
    wxString str;
    if (!arc.Get(name, str))
        return false;

    std::basic_istringstream<wxChar> ss(str.c_str());
    T val;
    ss >> val;

    if (ss)
        value = val;

    return ss != NULL;
}

} // namespace tt_solutions

#endif // ARCHIVE_H
