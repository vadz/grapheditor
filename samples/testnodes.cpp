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

namespace {

using tt_solutions::Factory;

// Import
Factory<ImportFileNode>::Impl   importfile(_T("importfile"));
Factory<ImportODBCNode>::Impl   importodbc(_T("importodbc"));

// Export
Factory<ExportFileNode>::Impl   exportfile(_T("exportfile"));
Factory<ExportODBCNode>::Impl   exportodbc(_T("exportodbc"));

// Analyse
Factory<SearchNode>::Impl       search(_T("search"));
Factory<SampleNode>::Impl       sample(_T("sample"));
Factory<SortNode>::Impl         sort(_T("sort"));
Factory<ValidateNode>::Impl     validate(_T("validate"));
Factory<AddressValNode>::Impl   addressval(_T("addressval"));

// Re-engineer
Factory<CleanNode>::Impl        clean(_T("clean"));
Factory<ExtractNode>::Impl      extract(_T("extract"));
Factory<SplitNode>::Impl        split(_T("split"));
Factory<UniteNode>::Impl        unite(_T("unite"));
Factory<InsertNode>::Impl       insert(_T("insert"));
Factory<DeleteNode>::Impl       del(_T("delete"));
Factory<ArrangeNode>::Impl      arrange(_T("arrange"));
Factory<AppendNode>::Impl       append(_T("append"));
Factory<SQLQueryNode>::Impl     sqlquery(_T("sqlquery"));

// Match
Factory<MatchNode>::Impl        match(_T("match"));
Factory<MatchTableNode>::Impl   matchtable(_T("matchtable"));
Factory<MergeNode>::Impl        merge(_T("merge"));

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
