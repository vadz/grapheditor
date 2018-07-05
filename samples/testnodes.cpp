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

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "testnodes.h"

/**
 * @file
 * @brief Definition of test nodes for the sample program.
 */

namespace {

using tt_solutions::Factory;

/**
 * Define factory objects creating all the different kinds of nodes.
 */
//@{

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

//@}

/**
 * Return the path of the directory containing the icons.
 */
wxString GetResourceDir()
{
    wxString dir;
    if (!wxGetEnv("WX_GRAPHTEST_DATA_DIR", &dir)) {
        dir = wxStandardPaths::Get().GetResourcesDir();
    }

    return dir + wxFileName::GetPathSeparator();
}

} // namespace

TestNode::TestNode(const wxColour& colour,
                   const wxString& operation,
                   const wxString& imgfile,
                   const wxString& rank)
  : ProjectNode(operation,
                wxEmptyString,
                wxEmptyString,
                wxIcon(GetResourceDir() + imgfile, wxBITMAP_TYPE_PNG),
                colour)
{
#ifdef FIXED_NODE_SIZE
    SetMaxAutoSize(wxSize());
#endif
    SetRank(rank);
}

void TestNode::OnLayout(wxDC& dc)
{
#ifdef FIXED_NODE_SIZE
    static wxSize defSize(3 * GetDPI().x / 2, GetDPI().y);

    if (GetSize() != defSize)
        SetSize(defSize);
    else
#endif
    ProjectNode::OnLayout(dc);
}
