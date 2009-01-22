/////////////////////////////////////////////////////////////////////////////
// Name:        testnodes.h
// Purpose:     Hierarchy of nodes for the test program
// Author:      Mike Wetherell
// Modified by:
// Created:     December 2008
// RCS-ID:      $Id$
// Copyright:   (c) TT-solutions
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/**
 * @file testnodes.h
 * @brief Hierarchy of nodes for the test program.
 */

#include "projectdesigner.h"

struct TestNode : datactics::ProjectNode
{
    TestNode(const wxColour& colour,
             const wxString& operation,
             const wxString& imgfile);
};

struct ImportNode : TestNode
{
    ImportNode(const wxString& operation = _T("Import"),
               const wxString& imgfile   = _T("import.png"))
    : TestNode(0x6bd79c, operation, imgfile)
    { }
};

struct ImportFileNode : ImportNode {
    ImportFileNode() : ImportNode(_T("Import File"), _T("importfile.png")) { }
};

struct ImportODBCNode : ImportNode {
    ImportODBCNode() : ImportNode(_T("Import ODBC"), _T("importfile.png")) { }
};

struct ExportNode : TestNode
{
    ExportNode(const wxString& operation = _T("Export"),
               const wxString& imgfile   = _T("export.png"))
    : TestNode(0x7b9af7, operation, imgfile)
    { }
};

struct ExportFileNode : ExportNode {
    ExportFileNode() : ExportNode(_T("Export File"), _T("exportfile.png")) { }
};

struct ExportODBCNode : ExportNode {
    ExportODBCNode() : ExportNode(_T("Export ODBC"), _T("exportfile.png")) { }
};

struct AnalyseNode : TestNode {
    AnalyseNode(const wxString& operation = _T("Analyse"),
                const wxString& imgfile   = _T("analyse.png"))
    : TestNode(0xd6aa6b, operation, imgfile)
    { }
};

struct SearchNode : AnalyseNode {
    SearchNode() : AnalyseNode(_T("Search"), _T("search.png")) { }
};

struct SampleNode : AnalyseNode {
    SampleNode() : AnalyseNode(_T("Sample"), _T("sample.png")) { }
};

struct SortNode : AnalyseNode {
    SortNode() : AnalyseNode(_T("Sort"), _T("sort.png")) { }
};

struct ValidateNode : AnalyseNode {
    ValidateNode() : AnalyseNode(_T("Validate"), _T("validate.png")) { }
};

struct AddressValNode : AnalyseNode {
    AddressValNode()
    : AnalyseNode(_T("Address Validation"), _T("addressval.png")) { }
};

struct ReEngNode : TestNode {
    ReEngNode(const wxString& operation = _T("Re-engineer"),
              const wxString& imgfile   = _T("reeng.png"))
    : TestNode(0xd686c6, operation, imgfile)
    { }
};

struct CleanNode : ReEngNode {
    CleanNode() : ReEngNode(_T("Clean"), _T("clean.png")) { }
};

struct ExtractNode : ReEngNode {
    ExtractNode() : ReEngNode(_T("Extract"), _T("extract.png")) { }
};

struct SplitNode : ReEngNode {
    SplitNode() : ReEngNode(_T("Split"), _T("split.png")) { }
};

struct UniteNode : ReEngNode {
    UniteNode() : ReEngNode(_T("Unite"), _T("unite.png")) { }
};

struct InsertNode : ReEngNode {
    InsertNode() : ReEngNode(_T("Insert"), _T("insert.png")) { }
};

struct DeleteNode : ReEngNode {
    DeleteNode() : ReEngNode(_T("Delete"), _T("delete.png")) { }
};

struct ArrangeNode : ReEngNode {
    ArrangeNode() : ReEngNode(_T("Arrange"), _T("arrange.png")) { }
};

struct AppendNode : ReEngNode {
    AppendNode() : ReEngNode(_T("Append"), _T("append.png")) { }
};

struct SQLQueryNode : ReEngNode {
    SQLQueryNode() : ReEngNode(_T("SQL Query"), _T("sqlquery.png")) { }
};

struct MatchUpNode : TestNode {
    MatchUpNode(const wxString& operation = _T("Match"),
                const wxString& imgfile   = _T("matchup.png"))
    : TestNode(0x7bdff7, operation, imgfile)
    { }
};

struct MatchNode : MatchUpNode {
    MatchNode() : MatchUpNode(_T("Match"), _T("match.png")) { }
};

struct MatchTableNode : MatchUpNode {
    MatchTableNode() : MatchUpNode(_T("Match Table"), _T("matchtbl.png")) { }
};

struct MergeNode : MatchUpNode {
    MergeNode() : MatchUpNode(_T("Merge"), _T("merge.png")) { }
};
