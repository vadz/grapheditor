/////////////////////////////////////////////////////////////////////////////
// Name:        archive.cpp
// Purpose:     Serialisation archive
// Author:      Mike Wetherell
// Modified by:
// Created:     January 2009
// RCS-ID:      $Id$
// Copyright:   (c) 2009 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <wx/xml/xml.h>

#include "archive.h"
#include "tie.h"

// ----------------------------------------------------------------------------
// Local definitions
// ----------------------------------------------------------------------------

namespace {

const wxChar *tagARCHIVE    = _T("archive");
const wxChar *tagID         = _T("id");
const wxChar *tagSORT       = _T("sort");

const wxString tagFONT      = _T("wxFont");

const wxChar *tagFACE       = _T("face");
const wxChar *tagPOINTS     = _T("points");
const wxChar *tagFAMILY     = _T("family");
const wxChar *tagSTYLE      = _T("style");
const wxChar *tagWEIGHT     = _T("weight");
const wxChar *tagUNDERLINE  = _T("underline");
const wxChar *tagENCODING   = _T("encoding");

wxString FontId(const wxString& desc)
{
    return tagFONT + _T(" ") + desc;
}

} // namespace

// ----------------------------------------------------------------------------
// Archive
// ----------------------------------------------------------------------------

namespace tt_solutions {

using std::pair;
using std::make_pair;

Archive::Archive()
  : m_storing(true)
{
}

Archive::~Archive()
{
    Clear();
}

void Archive::Clear()
{
    for (ItemMap::iterator it = m_items.begin(); it != m_items.end(); ++it)
        delete it->second;

    m_items.clear();
    m_sort.clear();
}

bool Archive::Load(wxInputStream& stream)
{
    m_storing = false;
    Clear();

    wxXmlDocument doc;

    if (!doc.Load(stream))
        return false;

    wxXmlNode *root = doc.GetRoot();

    if (root->GetName() != tagARCHIVE) {
        wxLogError(_("Error loading: unknown root element"));
        return false;
    }

    wxXmlNode *node = root->GetChildren();

    while (node) {
        wxString name = node->GetName();
        wxString id;

        if (node->GetPropVal(tagID, &id)) {
            wxString sortkey = node->GetPropVal(tagSORT, wxEmptyString);
            Item *item = Put(name, id, sortkey);

            if (item) {
                wxXmlNode *attrnode = node->GetChildren();

                while (attrnode) {
                    wxString pname = attrnode->GetName();
                    wxString value = attrnode->GetNodeContent();

                    if (!item->Put(pname, value))
                        wxLogError(_("Error loading <%s %s='%s'> ignoring duplicate <%s>"),
                                   name.c_str(), tagID,
                                   id.c_str(), pname.c_str());

                    attrnode = attrnode->GetNext();
                }
            }
            else {
                wxLogError(_("Error loading <%s %s='%s'> id is not unique"),
                           name.c_str(), tagID, id.c_str());
            }
        }
        else {
            wxLogError(_("Error loading <%s> missing %s"),
                       name.c_str(), tagID);
        }
        node = node->GetNext();
    }

    return true;
}

bool Archive::Save(wxOutputStream& stream) const
{
    wxXmlNode *root = new wxXmlNode(wxXML_ELEMENT_NODE, tagARCHIVE);
    ItemMap::const_iterator i;

    for (i = m_items.begin(); i != m_items.end(); ++i) {
        wxString id = i->first;
        const Item *item = i->second;
        wxString classname = item->GetClass();
        wxString sortkey = item->GetSort();

        wxXmlNode *node = new wxXmlNode(root, wxXML_ELEMENT_NODE, classname);
        node->AddProperty(tagID, id);
        if (!sortkey.empty())
            node->AddProperty(tagSORT, sortkey);

        Item::const_iterator j, jend;

        for (tie(j, jend) = item->GetAttribs(); j != jend; ++j) {
            wxString pname = j->first;
            wxString value = j->second;

            wxXmlNode *text = new wxXmlNode(node, wxXML_ELEMENT_NODE, pname);
            if (!value.empty())
                new wxXmlNode(text, wxXML_TEXT_NODE, wxEmptyString, value);
        }
    }

    wxXmlDocument doc;
    doc.SetRoot(root);
    return doc.Save(stream);
}

Archive::Item *Archive::Put(const wxString& name,
                            const wxString& id,
                            const wxString& sort)
{
    Item *item = new Item(*this, name, id, sort);
    bool added = m_items.insert(make_pair(id, item)).second;

    if (added) {
        if (m_sort.size() != 0)
            SortAdd(item);
        return item;
    }

    delete item;
    return NULL;
}

bool Archive::Remove(const wxString& id)
{
    ItemMap::iterator it = m_items.find(id);

    if (it == m_items.end())
        return false;

    SortRemove(it->second);
    delete it->second;
    m_items.erase(it);
    return true;
}

Archive::Item *Archive::Get(const wxString& id)
{
    ItemMap::iterator it = m_items.find(id);
    return it != m_items.end() ? it->second : NULL;
}

const Archive::Item *Archive::Get(const wxString& id) const
{
    ItemMap::const_iterator it = m_items.find(id);
    return it != m_items.end() ? it->second : NULL;
}

wxObject *Archive::GetInstance(const wxString& id) const {
    const Item *item = Get(id);
    return item ? item->GetInstance() : NULL;
}

wxString Archive::MakeId(const void *p)
{
    return wxString::Format(_T("%p"), p);
}

void Archive::Sort() const
{
    if (m_sort.size() == 0) {
        ItemMap::const_iterator it;
        for (it = m_items.begin(); it != m_items.end(); ++it)
            SortAdd(it->second);
    }

    wxASSERT(m_sort.size() == m_items.size());
}

void Archive::SortAdd(Item *item) const
{
    m_sort.insert(make_pair(item->GetSort(), item));
}

void Archive::SortRemove(Item *item) const
{
    wxString key = item->GetSort();
    iterator it, end;

    for (tie(it, end) = m_sort.equal_range(key); it != end; ++it) {
        if (it->second == item) {
            m_sort.erase(it);
            break;
        }
    }
}

void Archive::SortItem(Item& item, const wxString& key)
{
    if (&item.m_archive != this)
        return;

    if (m_sort.size() != 0) {
        SortRemove(&item);
        item.m_sort = key;
        SortAdd(&item);
    }
    else {
        item.m_sort = key;
    }
}

Archive::iterator_pair Archive::GetItems(const wxString& first)
{
    Sort();
    return make_pair(m_sort.lower_bound(first), m_sort.end());
}

Archive::const_iterator_pair Archive::GetItems(const wxString& first) const
{
    Sort();
    return make_pair(m_sort.lower_bound(first), m_sort.end());
}

// ----------------------------------------------------------------------------
// Archive::Item
// ----------------------------------------------------------------------------

Archive::Item::Item(Archive& archive,
                    const wxString& name,
                    const wxString& id,
                    const wxString& sort)
  : m_archive(archive),
    m_class(name),
    m_id(id),
    m_sort(sort),
    m_instance(NULL),
    m_owns(false)
{
}

bool Archive::Item::Put(const wxString& name, const wxString& value)
{
    return m_attribs.insert(make_pair(name, value)).second;
}

bool Archive::Item::Put(const wxString& name, const wxChar *value)
{
    return Put(name, wxString(value));
}

bool Archive::Item::Get(const wxString& name, wxString& value) const
{
    const_iterator it = m_attribs.find(name);
    if (it == m_attribs.end())
        return false;
    value = it->second;
    return true;
}

wxString Archive::Item::Get(const wxString& name) const
{
    return Get<wxString>(name);
}

bool Archive::Item::Has(const wxString& name) const
{
    return m_attribs.find(name) != m_attribs.end();
}

void Archive::Item::SetInstance(wxObject *instance, bool owns)
{
    if (m_owns)
        delete m_instance;

    m_instance = instance;
    m_owns = owns;
}

Archive::Item::iterator_pair Archive::Item::GetAttribs()
{
    return make_pair(m_attribs.begin(), m_attribs.end());
}

Archive::Item::const_iterator_pair Archive::Item::GetAttribs() const
{
    return make_pair(m_attribs.begin(), m_attribs.end());
}

// ----------------------------------------------------------------------------
// Insertors/Extractors
// ----------------------------------------------------------------------------

namespace {

template <class T>
bool PutPair(Archive::Item& arc, const wxString& name, const T& value)
{
    return arc.Put(name, wxString() << value.x << _T(",") << value.y);
}

template <class T>
bool GetPair(const Archive::Item& arc, const wxString& name, T& value)
{
    wxString str;
    if (!arc.Get(name, str))
        return false;

    int x, y;
    if (wxSscanf(str.c_str(), _T("%d,%d"), &x, &y) != 2)
        return false;

    value.x = x;
    value.y = y;
    return true;
}

} // namespace

bool Insert(Archive::Item& arc, const wxString& name, const wxPoint& value)
{
    return PutPair(arc, name, value);
}

bool Extract(const Archive::Item& arc, const wxString& name, wxPoint& value)
{
    return GetPair(arc, name, value);
}

bool Insert(Archive::Item& arc, const wxString& name, const wxSize& value)
{
    return PutPair(arc, name, value);
}

bool Extract(const Archive::Item& arc, const wxString& name, wxSize& value)
{
    return GetPair(arc, name, value);
}

bool Insert(Archive::Item& arc, const wxString& name, const wxRect& value)
{
    wxString str;

    str << value.x << _T(",")
        << value.y << _T(",")
        << value.width << _T(",")
        << value.height;

    return arc.Put(name, str);
}

bool Extract(const Archive::Item& arc, const wxString& name, wxRect& value)
{
    wxString str;
    if (!arc.Get(name, str))
        return false;

    int x, y, w, h;
    if (wxSscanf(str.c_str(), _T("%d,%d,%d,%d"), &x, &y, &w, &h) != 4)
        return false;

    value = wxRect(x, y, w, h);
    return true;
}

bool Insert(Archive::Item& arc, const wxString& name, const wxColour& value)
{
    return arc.Put(name, value.GetAsString(wxC2S_HTML_SYNTAX));
}

bool Extract(const Archive::Item& arc, const wxString& name, wxColour& value)
{
    wxString str;
    if (!arc.Get(name, str))
        return false;

    wxColour colour(str);
    if (!colour.Ok())
        return false;

    value = colour;
    return true;
}

bool Insert(Archive::Item& arc, const wxString& name, const wxFont& value)
{
    wxString desc = value.GetNativeFontInfoDesc();
    if (!arc.Put(name, desc))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item *item = archive.Put(tagFONT, FontId(desc));

    if (item) {
        item->Put(tagFACE, value.GetFaceName());
        item->Put(tagPOINTS, value.GetPointSize());
        item->Put(tagFAMILY, value.GetFamily());
        item->Put(tagSTYLE, value.GetStyle());
        item->Put(tagWEIGHT, value.GetWeight());
        if (value.GetUnderlined()) item->Put(tagUNDERLINE);
        item->Put(tagENCODING, value.GetEncoding());
    }

    return true;
}

bool Extract(const Archive::Item& arc, const wxString& name, wxFont& value)
{
    wxString desc;
    if (!arc.Get(name, desc))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item* item = archive.Get(FontId(desc));

    if (!item)
        return false;

    wxFont *obj = item->GetInstance<wxFont>();
    if (obj) {
        value = *obj;
        return true;
    }

    wxFont font;

    if (!font.SetNativeFontInfo(desc))
        if (!font.Create(item->Get<int>(tagPOINTS),
                         item->Get<int>(tagFAMILY),
                         item->Get<int>(tagSTYLE),
                         item->Get<int>(tagWEIGHT),
                         item->Has(tagUNDERLINE),
                         item->Get(tagFACE),
                         wxFontEncoding(item->Get<int>(tagENCODING))))
            return false;

    item->SetInstance(new wxFont(font), true);
    value = font;
    return true;
}

} // namespace tt_solutions
