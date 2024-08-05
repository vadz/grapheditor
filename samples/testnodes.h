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
 * @file
 * @brief Hierarchy of nodes for the test program.
 */

#include "projectdesigner.h"

/**
 * Base class for test nodes.
 */
struct TestNode : datactics::ProjectNode
{
    /**
     * Ctor creating a test node from the given attributes.
     */
    TestNode(const wxColour& colour,
             const wxString& operation,
             const wxString& imgfile,
             const wxString& rank = wxEmptyString);
    void OnLayout(wxReadOnlyDC& dc);
};

/// Base class for import test nodes.
struct ImportNode : TestNode
{
    /// Default ctor creates a node of this class.
    ImportNode(const wxString& operation = _T("Import"),
               const wxString& imgfile   = _T("import.png"))
    : TestNode(0x6bd79c, operation, imgfile, _T("import"))
    { }
};

/// Test node for import from file operation.
struct ImportFileNode : ImportNode {
    ImportFileNode() : ImportNode(_T("Import File"), _T("importfile.png")) { }
};

/// Test node for import from ODBC operation.
struct ImportODBCNode : ImportNode {
    ImportODBCNode() : ImportNode(_T("Import ODBC"), _T("importfile.png")) { }
};

/// Base class for export test nodes.
struct ExportNode : TestNode
{
    /// Default ctor creates a node of this class.
    ExportNode(const wxString& operation = _T("Export"),
               const wxString& imgfile   = _T("export.png"))
    : TestNode(0x7b9af7, operation, imgfile, _T("export"))
    { }
};

/// Test node for file export operation.
struct ExportFileNode : ExportNode {
    ExportFileNode() : ExportNode(_T("Export File"), _T("exportfile.png")) { }
};

/// Test node for ODBC export operation.
struct ExportODBCNode : ExportNode {
    ExportODBCNode() : ExportNode(_T("Export ODBC"), _T("exportfile.png")) { }
};

/// Test node for analyse operation.
struct AnalyseNode : TestNode {
    /// Default ctor creates a node of this class.
    AnalyseNode(const wxString& operation = _T("Analyse"),
                const wxString& imgfile   = _T("analyse.png"))
    : TestNode(0xd6aa6b, operation, imgfile)
    { }
};

/// Test node for search operation.
struct SearchNode : AnalyseNode {
    SearchNode() : AnalyseNode(_T("Search"), _T("search.png")) { }
};

/// Test sample node.
struct SampleNode : AnalyseNode {
    SampleNode() : AnalyseNode(_T("Sample"), _T("sample.png")) { }
};

/// Test node for sort operation.
struct SortNode : AnalyseNode {
    SortNode() : AnalyseNode(_T("Sort"), _T("sort.png")) { }
};

/// Test node for validation operation.
struct ValidateNode : AnalyseNode {
    ValidateNode() : AnalyseNode(_T("Validate"), _T("validate.png")) { }
};

/// Test node for address validation.
struct AddressValNode : AnalyseNode {
    AddressValNode()
    : AnalyseNode(_T("Address Validation"), _T("addressval.png")) { }
};

/// Base class for a reengineering operation.
struct ReEngNode : TestNode {
    /// Default ctor creates a node of this class.
    ReEngNode(const wxString& operation = _T("Re-engineer"),
              const wxString& imgfile   = _T("reeng.png"))
    : TestNode(0xd686c6, operation, imgfile)
    { }
};

/// Test node for clean operation.
struct CleanNode : ReEngNode {
    CleanNode() : ReEngNode(_T("Clean"), _T("clean.png")) { }
};

/// Test extract node.
struct ExtractNode : ReEngNode {
    ExtractNode() : ReEngNode(_T("Extract"), _T("extract.png")) { }
};

/// Test node for split operation.
struct SplitNode : ReEngNode {
    SplitNode() : ReEngNode(_T("Split"), _T("split.png")) { }
};

/// Test node for unit operation.
struct UniteNode : ReEngNode {
    UniteNode() : ReEngNode(_T("Unite"), _T("unite.png")) { }
};

/// Test node for insert operation.
struct InsertNode : ReEngNode {
    InsertNode() : ReEngNode(_T("Insert"), _T("insert.png")) { }
};

/// Test node for delete operation.
struct DeleteNode : ReEngNode {
    DeleteNode() : ReEngNode(_T("Delete"), _T("delete.png")) { }
};

/// Test arrange node.
struct ArrangeNode : ReEngNode {
    ArrangeNode() : ReEngNode(_T("Arrange"), _T("arrange.png")) { }
};

/// Test node for append operation.
struct AppendNode : ReEngNode {
    AppendNode() : ReEngNode(_T("Append"), _T("append.png")) { }
};

/// Test node for SQL query operation.
struct SQLQueryNode : ReEngNode {
    SQLQueryNode() : ReEngNode(_T("SQL Query"), _T("sqlquery.png")) { }
};

/// Base class for matching test nodes.
struct MatchUpNode : TestNode {
    /// Default ctor creates a node of this class.
    MatchUpNode(const wxString& operation = _T("Match"),
                const wxString& imgfile   = _T("matchup.png"))
    : TestNode(0x7bdff7, operation, imgfile)
    { }
};

/// Test node for match operation.
struct MatchNode : MatchUpNode {
    MatchNode() : MatchUpNode(_T("Match"), _T("match.png")) { }
};

/// Test node for table match operation.
struct MatchTableNode : MatchUpNode {
    MatchTableNode() : MatchUpNode(_T("Match Table"), _T("matchtbl.png")) { }
};

/// Test node for merge operation.
struct MergeNode : MatchUpNode {
    MergeNode() : MatchUpNode(_T("Merge"), _T("merge.png")) { }
};
