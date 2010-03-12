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

#define _CRT_SECURE_NO_WARNINGS

#if NO_EXPAT
#include <wx/xml/xml.h>
#else
#include <expat.h>
#endif

#include <wx/mstream.h>

#include "archive.h"
#include "base64.h"
#include "tie.h"

namespace tt_solutions {

namespace {

// ----------------------------------------------------------------------------
// Local definitions
// ----------------------------------------------------------------------------

const wxChar *TAGARCHIVE    = _T("archive");
const wxChar *TAGID         = _T("id");
const wxChar *TAGSORT       = _T("sort");

const wxString TAGFONT      = _T("wxFont");

const wxChar *TAGFACE       = _T("face");
const wxChar *TAGPOINTS     = _T("points");
const wxChar *TAGFAMILY     = _T("family");
const wxChar *TAGSTYLE      = _T("style");
const wxChar *TAGWEIGHT     = _T("weight");
const wxChar *TAGUNDERLINE  = _T("underline");
const wxChar *TAGENCODING   = _T("encoding");

const wxString TAGIMAGE     = _T("wxImage");
const wxChar *TAGBASE64     = _T("base64");

wxString FontId(const wxString& desc)
{
    return TAGFONT + _T(" ") + desc;
}

// ----------------------------------------------------------------------------
// XML parser
// ----------------------------------------------------------------------------

#ifndef NO_EXPAT

class Parser
{
public:
    Parser(Archive *archive);

    void StartElement(const XML_Char *name, const XML_Char **atts);
    void EndElement(const XML_Char *name);
    void CharData(const XML_Char *s, int len);

protected:
    wxString FromXml(const char *str, size_t len = wxString::npos);
    wxString FromXml(const wchar_t *str, size_t len = wxString::npos);

private:
    int m_depth;

    Archive *m_archive;
    Archive::Item *m_item;
    wxString m_value;
};

Parser::Parser(Archive *archive)
  : m_depth(0),
    m_archive(archive),
    m_item(NULL)
{
}

wxString Parser::FromXml(const wchar_t *str, size_t len)
{
    return wxString(str, *wxConvUI, len);
}

wxString Parser::FromXml(const char *str, size_t len)
{
    size_t wlen;
    wxWCharBuffer wbuf = wxConvUTF8.cMB2WC(str, len, &wlen);
    return FromXml(wbuf, wlen);
}

void Parser::StartElement(const XML_Char *name, const XML_Char **atts)
{
    m_depth++;

    if (m_depth == 1) {
        if (FromXml(name) != TAGARCHIVE) {
            wxLogError(_("Error loading: unknown root element"));
        }
    }
    else if (m_depth == 2) {
        wxASSERT(m_item == NULL);
        wxString classname = FromXml(name);
        wxString id;
        wxString sortkey;

        while (*atts) {
            wxString atname = FromXml(*atts++);
            wxString atvalue = FromXml(*atts++);

            if (atname == TAGID)
                id = atvalue;
            else if (atname == TAGSORT)
                sortkey = atvalue;
        }

        if (id.empty()) {
            wxLogError(_("Error loading <%s> missing %s"),
                       classname.c_str(), TAGID);
        }
        else {
            m_item = m_archive->Put(classname, id, sortkey);

            if (!m_item) {
                wxLogError(_("Error loading <%s %s='%s'> id is not unique"),
                           classname.c_str(), TAGID, id.c_str());
            }
        }
    }
}

void Parser::EndElement(const XML_Char *name)
{
    if (m_depth == 2) {
        m_item = NULL;
    }
    else if (m_depth == 3) {
        wxString pname = FromXml(name);
        if (m_item && !m_item->Put(pname, m_value)) {
            wxLogError(_("Error loading <%s %s='%s'> ignoring duplicate <%s>"),
                       m_item->GetClass().c_str(), TAGID,
                       m_item->GetId().c_str(), pname.c_str());
        }
        m_value.clear();
    }

    m_depth--;
}

void Parser::CharData(const XML_Char *s, int len)
{
    if (m_depth >= 3)
        m_value += FromXml(s, len);
}

extern "C" {

void //XMLCALL
start_element(void *userData, const XML_Char *name, const XML_Char **atts)
{
    static_cast<Parser*>(userData)->StartElement(name, atts);
}

void //XMLCALL
end_element(void *userData, const XML_Char *name)
{
    static_cast<Parser*>(userData)->EndElement(name);
}

void //XMLCALL
char_data(void *userData, const XML_Char *s, int len)
{
    static_cast<Parser*>(userData)->CharData(s, len);
}

} // extern "C"

#endif // NO_EXPAT

// ----------------------------------------------------------------------------
// XML generator
// ----------------------------------------------------------------------------

class Generator
{
public:
    Generator(wxOutputStream& out);

    void Write(const wxString& str);
    void Write(const char *utf, size_t len = wxString::npos);

    void Pair(const wxString& name_and_attrs, const wxString& value);
    void Start(const wxString& name_and_attrs);
    void End(const wxString& name);
    void CharData(const wxString& str);

protected:
    wxString FromXml(const char *str, size_t len = wxString::npos);
    wxString FromXml(const wchar_t *str, size_t len = wxString::npos);

private:
    int m_depth;
    bool m_leaf;
    wxOutputStream& m_stream;
};

Generator::Generator(wxOutputStream& stream)
  : m_depth(0),
    m_leaf(false),
    m_stream(stream)
{
}

void Generator::Write(const char *utf, size_t len)
{
    if (len == wxString::npos)
        len = strlen(utf);
    m_stream.Write(utf, len);
}

void Generator::Write(const wxString& str)
{
#if wxUSE_UNICODE
    size_t clen;
    wxCharBuffer cbuf = wxConvUTF8.cWC2MB(str, str.length(), &clen);
#else
    size_t wlen, clen;
    wxWCharBuffer wbuf = wxConvUI->cMB2WC(str, str.length(), &wlen);
    wxCharBuffer cbuf = wxConvUTF8.cWC2MB(wbuf, wlen, &clen);
#endif
    Write(cbuf, clen);
}

void Generator::Pair(const wxString& name_and_attrs, const wxString& value)
{
    if (value.empty()) {
        Start(name_and_attrs + _T("/"));
        End(wxEmptyString);
    }
    else {
        Start(name_and_attrs);
        CharData(value);
        End(name_and_attrs.BeforeFirst(_T(' ')));
    }
}

void Generator::Start(const wxString& name_and_attrs)
{
    m_leaf = true;

    wxString buf;
    buf << _T("\n") << wxString(_T(' '), m_depth * 2)
        << _T("<") << name_and_attrs << _T(">");
    Write(buf);

    m_depth++;
}

void Generator::End(const wxString& name)
{
    m_depth--;

    if (!name.empty()) {
        wxString buf;
        if (!m_leaf)
            buf << _T("\n") << wxString(_T(' '), m_depth * 2);
        buf << _T("</") + name + _T(">");
        Write(buf);
    }

    m_leaf = false;
}

void Generator::CharData(const wxString& str)
{
    wxString::const_iterator it = str.begin();
    wxString buf;
    int square = 0;

    while (it != str.end()) {
        wxChar ch = *it++;
        switch (ch) {
            case _T('&'):
                buf += _T("&amp;");
                break;
            case _T('<'):
                buf += _T("&lt;");
                break;
            case _T('>'):
                if (square >= 2)
                    buf += _T("&gt;");
                else
                    buf += ch;
                break;
            default:
                buf += ch;
        }
        square = ch == _T(']') ? square + 1 : 0;

        if (buf.length() >= 8192) {
            Write(buf);
            buf.clear();
        }
    }

    if (!buf.empty())
        Write(buf);
}

wxString Attribute(const wxString& name, const wxString& str)
{
    wxString::const_iterator it = str.begin();
    wxString attr;

    attr += _T(" ");
    attr += name;
    attr += _T("=\"");

    while (it != str.end()) {
        wxChar ch = *it++;
        switch (ch) {
            case _T('<'): attr += _T("&lt;"); break;
            case _T('&'): attr += _T("&amp;"); break;
            case _T('"'): attr += _T("&quot;"); break;
            default:      attr += ch;
        }
    }

    attr += _T("\"");
    return attr;
}

} // namespace

// ----------------------------------------------------------------------------
// Archive
// ----------------------------------------------------------------------------

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

#ifdef NO_EXPAT

namespace {

#if wxUSE_UNICODE

wxString FromXml(const wxString& str)  { return str; }

#else

wxString FromXml(const wxString& str)
{
    return wxConvUI->cWC2MB(wxConvUTF8.cMB2WC(str));
}

#endif

#if wxCHECK_VERSION(2, 9, 0)

bool GetAttribute(const wxXmlNode *node, const wxString& name, wxString *value)
{
    return node->GetAttribute(name, value);
}

#else

bool GetAttribute(const wxXmlNode *node, const wxString& name, wxString *value)
{
    return node->GetPropVal(name, value);
}

#endif

} // namespace

bool Archive::Load(wxInputStream& stream)
{
    m_storing = false;
    Clear();

    wxXmlDocument doc;

    if (!doc.Load(stream))
        return false;

    wxXmlNode *root = doc.GetRoot();

    if (root->GetName() != TAGARCHIVE) {
        wxLogError(_("Error loading: unknown root element"));
        return false;
    }

    wxXmlNode *node = root->GetChildren();

    while (node) {
        wxString name = node->GetName();
        wxString id;

        if (GetAttribute(node, TAGID, &id)) {
            wxString sortkey;
            GetAttribute(node, TAGSORT, &sortkey);
            Item *item = Put(name, id, sortkey);

            if (item) {
                wxXmlNode *attrnode = node->GetChildren();

                while (attrnode) {
                    wxString pname = attrnode->GetName();
                    wxString value = FromXml(attrnode->GetNodeContent());

                    if (!item->Put(pname, value))
                        wxLogError(_("Error loading <%s %s='%s'> ignoring duplicate <%s>"),
                                   name.c_str(), TAGID,
                                   id.c_str(), pname.c_str());

                    attrnode = attrnode->GetNext();
                }
            }
            else {
                wxLogError(_("Error loading <%s %s='%s'> id is not unique"),
                           name.c_str(), TAGID, id.c_str());
            }
        }
        else {
            wxLogError(_("Error loading <%s> missing %s"),
                       name.c_str(), TAGID);
        }
        node = node->GetNext();
    }

    return true;
}

#else // NO_EXPAT

bool Archive::Load(wxInputStream& stream)
{
    m_storing = false;
    Clear();

    Parser handler(this);
    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &handler);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, char_data);

    const size_t bufsize = 8192;
    wxCharBuffer buf(bufsize);
    XML_Status status = XML_STATUS_OK;

    size_t len;
    while (status == XML_STATUS_OK &&
            (len = stream.Read(buf.data(), bufsize).LastRead()) != 0)
        status = XML_Parse(parser, buf, len, len < bufsize);

    XML_ParserFree(parser);
    return status == XML_STATUS_OK;
}

#endif // NO_EXPAT

bool Archive::Save(wxOutputStream& stream) const
{
    Generator out(stream);
    out.Write("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
    out.Start(TAGARCHIVE);

    ItemMap::const_iterator i;

    for (i = m_items.begin(); i != m_items.end(); ++i) {
        wxString id = i->first;
        const Item *item = i->second;
        wxString classname = item->GetClass();
        wxString sortkey = item->GetSort();

        wxString attrs = Attribute(TAGID, id);
        if (!sortkey.empty())
            attrs += Attribute(TAGSORT, sortkey);
        out.Start(classname + attrs);

        Item::const_iterator j, jend;

        for (tie(j, jend) = item->GetAttribs(); j != jend; ++j) {
            wxString pname = j->first;
            wxString value = j->second;

            out.Pair(pname, value);
        }

        out.End(classname);
    }

    out.End(TAGARCHIVE);
    out.Write("\n");

    return stream.IsOk();
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

Archive::iterator_pair Archive::DoGetItems(const wxString& prefix) const
{
    Sort();

    if (prefix.empty())
        return make_pair(m_sort.begin(), m_sort.end());

    wxString last = prefix;
    wxString::reference ch = *last.rbegin();
    ch = wxChar(ch) + 1;

    return make_pair(m_sort.lower_bound(prefix), m_sort.lower_bound(last));
}

Archive::iterator_pair Archive::GetItems(const wxString& prefix)
{
    return DoGetItems(prefix);
}

Archive::const_iterator_pair Archive::GetItems(const wxString& prefix) const
{
    return DoGetItems(prefix);
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

bool Archive::Item::Remove(const wxString& name)
{
    return m_attribs.erase(name) > 0;
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
    Archive::Item *item = archive.Put(TAGFONT, FontId(desc));

    if (item) {
        item->Put(TAGFACE, value.GetFaceName());
        item->Put(TAGPOINTS, value.GetPointSize());
        item->Put(TAGFAMILY, value.GetFamily());
        item->Put(TAGSTYLE, value.GetStyle());
        item->Put(TAGWEIGHT, value.GetWeight());
        if (value.GetUnderlined()) item->Put(TAGUNDERLINE);
        item->Put(TAGENCODING, value.GetEncoding());
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

    //if (!font.SetNativeFontInfo(desc))
        if (!font.Create(item->Get<int>(TAGPOINTS),
                         wxFontFamily(item->Get<int>(TAGFAMILY)),
                         wxFontStyle(item->Get<int>(TAGSTYLE)),
                         wxFontWeight(item->Get<int>(TAGWEIGHT)),
                         item->Has(TAGUNDERLINE),
                         item->Get(TAGFACE)))
                         //wxFontEncoding(item->Get<int>(TAGENCODING))))
            return false;

    item->SetInstance(new wxFont(font), true);
    value = font;
    return true;
}

namespace {

void PutImage(Archive::Item *item, const wxImage& img, wxBitmapType type)
{
    wxString value;

    {
        wxCharBuffer buf;
        size_t len;

        {
            wxMemoryOutputStream stream;
            img.SaveFile(stream, type);

            len = size_t(stream.GetLength());
            buf = wxCharBuffer(len);

            stream.CopyTo(buf.data(), len);
        }

        value = wxBase64Encode(buf, len);
    }

    item->Put(TAGBASE64, value);
}

wxImage GetImage(Archive::Item *item)
{
    wxMemoryBuffer buf = wxBase64Decode(item->Get(TAGBASE64));
    wxImage img;
    wxMemoryInputStream stream(buf, buf.GetDataLen());
    img.LoadFile(stream);
    return img;
}

} // namespace

bool Insert(Archive::Item& arc,
            const wxString& name,
            const wxIcon& value,
            wxBitmapType type)
{
    wxString id = Archive::MakeId(value.GetRefData());
    if (!arc.Put(name, id))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item *item = archive.Put(TAGIMAGE, id);

    if (item) {
        wxImage img;

        {
            wxBitmap bmp;
            bmp.CopyFromIcon(value);
            img = bmp.ConvertToImage();
        }

        PutImage(item, img, type);
    }

    return true;
}

bool Extract(const Archive::Item& arc,
             const wxString& name,
             wxIcon& value,
             wxBitmapType)
{
    wxString id;
    if (!arc.Get(name, id))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item* item = archive.Get(id);

    if (!item)
        return false;

    wxIcon *obj = item->GetInstance<wxIcon>();
    if (obj) {
        value = *obj;
        return true;
    }

    wxIcon icon;

    {
        wxBitmap bmp(GetImage(item));
        icon.CopyFromBitmap(bmp);
    }

    item->SetInstance(new wxIcon(icon), true);
    value = icon;
    return true;
}

bool Insert(Archive::Item& arc,
            const wxString& name,
            const wxBitmap& value,
            wxBitmapType type)
{
    wxString id = Archive::MakeId(value.GetRefData());
    if (!arc.Put(name, id))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item *item = archive.Put(TAGIMAGE, id);

    if (item)
        PutImage(item, value.ConvertToImage(), type);

    return true;
}

bool Extract(const Archive::Item& arc,
             const wxString& name,
             wxBitmap& value,
             wxBitmapType)
{
    wxString id;
    if (!arc.Get(name, id))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item* item = archive.Get(id);

    if (!item)
        return false;

    wxBitmap *obj = item->GetInstance<wxBitmap>();
    if (obj) {
        value = *obj;
        return true;
    }

    wxBitmap bmp(GetImage(item));

    item->SetInstance(new wxBitmap(bmp), true);
    value = bmp;
    return true;
}

bool Insert(Archive::Item& arc,
            const wxString& name,
            const wxImage& value,
            wxBitmapType type)
{
    wxString id = Archive::MakeId(value.GetRefData());
    if (!arc.Put(name, id))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item *item = archive.Put(TAGIMAGE, id);

    if (item)
        PutImage(item, value, type);

    return true;
}

bool Extract(const Archive::Item& arc,
             const wxString& name,
             wxImage& value,
             wxBitmapType)
{
    wxString id;
    if (!arc.Get(name, id))
        return false;

    Archive& archive = arc.GetArchive();
    Archive::Item* item = archive.Get(id);

    if (!item)
        return false;

    wxImage *obj = item->GetInstance<wxImage>();
    if (obj) {
        value = *obj;
        return true;
    }

    wxImage img = GetImage(item);

    item->SetInstance(new wxImage(img), true);
    value = img;
    return true;
}

} // namespace tt_solutions
