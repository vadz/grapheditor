/////////////////////////////////////////////////////////////////////////////
// Name:        testnodes.cpp
// Purpose:     Hierarchy of nodes for the test program
// Author:      Mike Wetherell
// Modified by:
// Created:     December 2008
// RCS-ID:      $Id$
// Copyright:   (c) TT-solutions
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/**
 * @file testnodes.cpp
 * @brief Hierarchy of nodes for the test program.
 */

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "testnodes.h"

using tt_solutions::Factory;

// Import
template <> Factory<ImportFileNode>::Impl
    Factory<ImportFileNode>::sm_impl(_T("importfile"));
template <> Factory<ImportODBCNode>::Impl
    Factory<ImportODBCNode>::sm_impl(_T("importodbc"));

// Export
template <> Factory<ExportFileNode>::Impl
    Factory<ExportFileNode>::sm_impl(_T("exportfile"));
template <> Factory<ExportODBCNode>::Impl
    Factory<ExportODBCNode>::sm_impl(_T("exportodbc"));

// Analyse
template <> Factory<SearchNode>::Impl
    Factory<SearchNode>::sm_impl(_T("search"));
template <> Factory<SampleNode>::Impl
    Factory<SampleNode>::sm_impl(_T("sample"));
template <> Factory<SortNode>::Impl
    Factory<SortNode>::sm_impl(_T("sort"));
template <> Factory<ValidateNode>::Impl
    Factory<ValidateNode>::sm_impl(_T("validate"));
template <> Factory<AddressValNode>::Impl
    Factory<AddressValNode>::sm_impl(_T("addressval"));

// Re-engineer
template <> Factory<CleanNode>::Impl
    Factory<CleanNode>::sm_impl(_T("clean"));
template <> Factory<ExtractNode>::Impl
    Factory<ExtractNode>::sm_impl(_T("extract"));
template <> Factory<SplitNode>::Impl
    Factory<SplitNode>::sm_impl(_T("split"));
template <> Factory<UniteNode>::Impl
    Factory<UniteNode>::sm_impl(_T("unite"));
template <> Factory<InsertNode>::Impl
    Factory<InsertNode>::sm_impl(_T("insert"));
template <> Factory<DeleteNode>::Impl
    Factory<DeleteNode>::sm_impl(_T("delete"));
template <> Factory<ArrangeNode>::Impl
    Factory<ArrangeNode>::sm_impl(_T("arrange"));
template <> Factory<AppendNode>::Impl
    Factory<AppendNode>::sm_impl(_T("append"));
template <> Factory<SQLQueryNode>::Impl
    Factory<SQLQueryNode>::sm_impl(_T("sqlquery"));

// Match
template <> Factory<MatchNode>::Impl
    Factory<MatchNode>::sm_impl(_T("match"));
template <> Factory<MatchTableNode>::Impl
    Factory<MatchTableNode>::sm_impl(_T("matchtable"));
template <> Factory<MergeNode>::Impl
    Factory<MergeNode>::sm_impl(_T("merge"));

namespace {

wxString GetResourceDir()
{
    wxChar sep = wxFileName::GetPathSeparator();
    wxString exedir = wxStandardPaths().GetExecutablePath().BeforeLast(sep);

#ifdef RESOURCE_DIR
    return exedir + sep + RESOURCE_DIR + sep;
#else
    return exedir.BeforeLast(sep).BeforeLast(sep) + sep +
           _T("samples") + sep + _T("resources") + sep;
#endif
}

} // namespace

TestNode::TestNode(const wxColour& colour,
                   const wxString& operation,
                   const wxString& imgfile)
  : ProjectNode(operation,
                wxEmptyString,
                wxEmptyString,
                wxIcon(GetResourceDir() + imgfile, wxBITMAP_TYPE_PNG),
                colour)
{
}
