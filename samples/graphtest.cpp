/////////////////////////////////////////////////////////////////////////////
// Name:        graphtest.cpp
// Purpose:     Example program for the graph editor
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) TT-solutions
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/**
 * @file graphtest.cpp
 * @brief Sample program for the graph editor.
 */

/**
 * @mainpage
 *
 * The following code shows how to create graph control:
 *
 * @code
 *  m_graphctrl = new ProjectDesigner(splitter);
 *  m_graph = new Graph;
 *  m_graphctrl->SetGraph(m_graph);
 *  m_graph->SetEventHandler(this);
 * @endcode
 *
 * You create a <code>GraphCtrl</code> (or <code>ProjectDesigner</code> in
 * this case, derived from <code>GraphCtrl</code>), a <code>Graph</code> and
 * associate the two with <code>SetGraph()</code>.
 *
 * The reason for the spit is so that (potentially at least) in a doc/view
 * application the doc could own a <code>Graph</code> while multiple views
 * could use multiple <code>GraphCtrl</code> to allow editing of it. This
 * isn't supported at the moment, and the two must be used in a one-to-one
 * pair.
 *
 * Both the <code>Graph</code> and the <code>GraphCtrl</code> can fire
 * events. The <code>GraphCtrl</code> ones can be handled by the
 * <code>GraphCtrl</code>'s parent as usual, but for the <code>Graph</code>
 * (which is not a window and does not have a parent) it is necessary to call
 * <code>SetEventHandler()</code> as shown to select the class that will
 * handle its events.
 *
 * Nodes (these are of type <code>GraphNode</code> or derived from it), can
 * be added to the graph using <code>Graph::Add()</code>. In the graphtest
 * sample program this is done in response to <code>EVT_GRAPHTREE_DROP</code>
 * events from the tree control, see <code>OnGraphTreeDrop()</code> in
 * graphtest.cpp for an example.
 *
 * Edges are of type <code>GraphEdge</code>, and there is also an overload of
 * Graph::Add() for adding these, however it's not usually necessary to use
 * it since the <code>GraphCtrl</code> will add edges itself when the user
 * drags one node onto another.  This processes can be controlled using the
 * <code>EVT_GRAPH_CONNECT_FEEDBACK</code> event, which is fired during
 * dragging, and vetoing it disallows a connection from being made.
 *
 * The cursor positions returned by the event object's
 * <code>GetPosition()</code> are in the coordinate system of the
 * <code>Graph</code>, and so can be passed to <code>Graph</code>'s methods
 * without conversion. To use the position with other classes, for example to
 * display a popup menu in response to a <code>EVT_GRAPH_NODE_MENU</code>
 * event, you can convert it using <code>GraphCtrl::GraphToScreen()</code>.
 * See <code>OnMenuNode()</code> in graphtest.cpp for an example of this.
 * Given a cursor position it is possible to tell which part of a
 * <code>ProjectNode</code> has been clicked using its <code>HitTest()</code>
 * method, see <code>OnActivateNode()</code> in graphtest.cpp for an example
 * of this.
 *
 * The classes <code>GraphNode</code> and <code>GraphEdge</code> have a
 * common base class <code>GraphElement</code>.  Graph elements are
 * enumerated using iterators. The iterators are assignable if references to
 * the types they point to would be assignable.  E.g. if a method needs a
 * pair of GraphElement iterators, then you can pass a pair of
 * <code>GraphNode</code> iterators (since a <code>GraphNode</code> is-a
 * <code>GraphElement</code>).
 *
 * Methods that return iterators return a begin/end pair in a
 * <code>std::pair</code>.  These can be assigned to a pair of variables
 * using the '<code>tie</code>' function, so the usual idiom for using them
 * is:
 *
 * @code
 *  Graph::node_iterator it, end;
 *
 *  for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it)
 *      it->SetSize(size);
 * @endcode
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/splitter.h>
#include <wx/numdlg.h>
#include <wx/colordlg.h>
#include <wx/fontdlg.h>
#include <wx/filename.h>
#include <wx/wxhtml.h>
#include <wx/imaglist.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>

#include <vector>

#include "graphtree.h"
#include "graphprint.h"
#include "testnodes.h"

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources and even
// though we could still include the XPM here it would be unused)
#if !defined(__WXMSW__) && !defined(__WXPM__)
    #include "graphtest.xpm"
#endif

const wxChar *helptext =
_T("<html>                                                                  ")
_T("  <head>                                                                ")
_T("    <title>graphtest</title>                                            ")
_T("  </head>                                                               ")
_T("  <body>                                                                ")
_T("    <h3>Adding Nodes</h3>                                               ")
_T("                                                                        ")
_T("    <p>To add a node, double click a leaf of the tree control, or drag  ")
_T("    it onto the graph editor.                                           ")
_T("                                                                        ")
_T("    <h3>Adding Links</h3>                                               ")
_T("                                                                        ")
_T("    <p>Links can be created between two nodes by dragging one node      ")
_T("    onto the other. During dragging the outline rectangle will become   ")
_T("    a line if dropping here is allowed and will create a link.          ")
_T("                                                                        ")
_T("    <p>This example program disallows incoming links to 'Import' nodes  ")
_T("    and outgoing links from 'Export' nodes.                             ")
_T("                                                                        ")
_T("    <h3>Panning</h3>                                                    ")
_T("                                                                        ")
_T("    <p>The graph control can pan over the graph by shift dragging the   ")
_T("    background.                                                         ")
_T("                                                                        ")
_T("    <h3>Selection</h3>                                                  ")
_T("                                                                        ")
_T("    <p>Mutliple selection is possible by rubber-banding (dragging a     ")
_T("    rectangle over the background), or by ctrl+clicking graph           ")
_T("    elements.                                                           ")
_T("                                                                        ")
_T("    <h3>Context Menu</h3>                                               ")
_T("                                                                        ")
_T("    <p>Right clicking a graph element brings up a context menu for the  ")
_T("    current selection.                                                  ")
_T("                                                                        ")
_T("    <h3>Activation</h3>                                                 ")
_T("                                                                        ")
_T("    <p>Nodes can be activated by double clicking them.                  ")
_T("                                                                        ")
_T("    <h3>Grid Spacing</h3>                                               ")
_T("                                                                        ")
_T("    <p>When the graph control displays a grid, it draws one line for    ")
_T("    every five gridlines of the associated graph.                       ")
_T("                                                                        ")
_T("  </body>                                                               ")
_T("</html>                                                                 ")
;

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

using datactics::ProjectDesigner;
using datactics::ProjectNode;

using namespace tt_solutions;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class TreeItemData : public wxTreeItemData
{
public:
    TreeItemData(const Factory<ProjectNode>& factory) : m_factory(factory) { }
    ProjectNode *New() const { return m_factory.New(); }
private:
    const Factory<ProjectNode> m_factory;
};

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit();
    virtual int OnExit();
};

// Define a new frame type: this is going to be our main frame
class MyFrame : public wxFrame
{
public:
    // ctor(s) and dtor
    MyFrame(const wxString& title);
    ~MyFrame();

    // tree control events
    void OnGraphTreeDrop(GraphTreeEvent& event);
    void OnTreeItemActivated(wxTreeEvent& event);

    // graph events
    void OnAddNode(GraphEvent& event);
    void OnDeleteNode(GraphEvent& event);
    void OnAddEdge(GraphEvent& event);
    void OnDeleteEdge(GraphEvent& event);
    void OnConnectFeedback(GraphEvent& event);
    void OnConnect(GraphEvent& event);

    // graph control events
    void OnClickNode(GraphEvent& event);
    void OnActivateNode(GraphEvent& event);
    void OnMenuNode(GraphEvent& event);
    void OnClickEdge(GraphEvent& event);
    void OnActivateEdge(GraphEvent& event);
    void OnMenuEdge(GraphEvent& event);

    // file menu
    void OnNew(wxCommandEvent &event);
    void OnOpen(wxCommandEvent &event);
    void OnSave(wxCommandEvent& event);
    void OnSaveImage(wxCommandEvent& event);
    void OnPrint(wxCommandEvent &event);
    void OnPreview(wxCommandEvent &event);
    void OnPrintSetup(wxCommandEvent &event);
    void OnPrintScaling(wxCommandEvent&);
    void OnQuit(wxCommandEvent& event);

    // edit menu
    void OnCut(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);

    // test menu
    void OnLayoutAll(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);
    void OnSetZoom(wxCommandEvent&);
    void OnShowGrid(wxCommandEvent& event);
    void OnUIShowGrid(wxUpdateUIEvent& event);
    void OnSnapToGrid(wxCommandEvent&);
    void OnUISnapToGrid(wxUpdateUIEvent& event);
    void OnSetGrid(wxCommandEvent&);

    // help menu
    void OnHelp(wxCommandEvent&);
    void OnAbout(wxCommandEvent& event);

    // context menu
    void OnSetSize(wxCommandEvent& event);
    void OnLayout(wxCommandEvent& event);
    void OnSetFont(wxCommandEvent& event);
    void OnSetColour(wxCommandEvent& event);
    void OnSetBgColour(wxCommandEvent& event);
    void OnSetTextColour(wxCommandEvent& event);
    void OnSetStyle(wxCommandEvent&);
    void OnSetLineStyle(wxCommandEvent&);
    void OnSetBorderThickness(wxCommandEvent& event);
    void OnSetCornerRadius(wxCommandEvent& event);

    wxString TextPrompt(const wxString& prompt, const wxString& value);
    ProjectNode *NewNode(const wxTreeItemId& id, const wxPoint& pt);
    bool PickFile(int flags);

    template <class T> void AppendTreeItem(const wxTreeItemId& id);

    wxTreeItemId AppendTreeItem(const wxTreeItemId& id,
                                const ProjectNode& node,
                                TreeItemData *tid = NULL);

    enum {
        ZoomMin = 25,
        ZoomMax = 300,
        ZoomStep = 25
    };

private:
    GraphTreeCtrl *m_tree;
    ProjectDesigner *m_graphctrl;
    GraphElement *m_element;
    GraphEdge *m_edge;
    GraphNode *m_node;
    Graph *m_graph;
    wxString m_filename;

    // the application's printer setup
    wxPrintDialogData m_printDialogData;
    // the application's page setup
    wxPageSetupDialogData m_pageSetupDialogData;

    MaxPages m_pages;
    double m_printscale;

    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum {
    ID_SAVEIMAGE,
    ID_PRINT_SCALING,
    ID_LIMIT_PAGES,
    ID_LAYOUTALL,
    ID_SHOWGRID,
    ID_SNAPTOGRID,
    ID_SETGRID,
    ID_ZOOM,
    ID_LAYOUT,
    ID_SETSIZE,
    ID_SETFONT,
    ID_SETCOLOUR,
    ID_SETBGCOLOUR,
    ID_SETTEXTCOLOUR,
    ID_STYLE,
    ID_CUSTOM,
    ID_RECTANGLE,
    ID_ELIPSE,
    ID_TRIANGLE,
    ID_DIAMOND,
    ID_LINE,
    ID_ARROW,
    ID_SETBORDERTHCKNESS,
    ID_SETCORNERRADIUS
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
    EVT_MENU(wxID_NEW, MyFrame::OnNew)
    EVT_MENU(wxID_OPEN, MyFrame::OnOpen)
    EVT_MENU(wxID_SAVE, MyFrame::OnSave)
    EVT_MENU(wxID_SAVEAS, MyFrame::OnSave)
    EVT_MENU(ID_SAVEIMAGE, MyFrame::OnSaveImage)
    EVT_MENU(wxID_PRINT, MyFrame::OnPrint)
    EVT_MENU(wxID_PREVIEW, MyFrame::OnPreview)
    EVT_MENU(wxID_PRINT_SETUP, MyFrame::OnPrintSetup)
    EVT_MENU(ID_PRINT_SCALING, MyFrame::OnPrintScaling)

    EVT_MENU(wxID_CUT, MyFrame::OnCut)
    EVT_MENU(wxID_COPY, MyFrame::OnCopy)
    EVT_MENU(wxID_PASTE, MyFrame::OnPaste)
    EVT_MENU(wxID_CLEAR, MyFrame::OnClear)
    EVT_MENU(wxID_SELECTALL, MyFrame::OnSelectAll)

    EVT_MENU(ID_LAYOUTALL, MyFrame::OnLayoutAll)
    EVT_MENU(wxID_ZOOM_IN, MyFrame::OnZoomIn)
    EVT_MENU(wxID_ZOOM_OUT, MyFrame::OnZoomOut)
    EVT_MENU(ID_ZOOM, MyFrame::OnSetZoom)
    EVT_MENU(ID_SHOWGRID, MyFrame::OnShowGrid)
    EVT_UPDATE_UI(ID_SHOWGRID, MyFrame::OnUIShowGrid)
    EVT_MENU(ID_SNAPTOGRID, MyFrame::OnSnapToGrid)
    EVT_UPDATE_UI(ID_SNAPTOGRID, MyFrame::OnUISnapToGrid)
    EVT_MENU(ID_SETGRID, MyFrame::OnSetGrid)

    EVT_MENU(ID_LAYOUT, MyFrame::OnLayout)
    EVT_MENU(ID_SETSIZE, MyFrame::OnSetSize)
    EVT_MENU(ID_SETFONT, MyFrame::OnSetFont)
    EVT_MENU(ID_SETCOLOUR, MyFrame::OnSetColour)
    EVT_MENU(ID_SETBGCOLOUR, MyFrame::OnSetBgColour)
    EVT_MENU(ID_SETTEXTCOLOUR, MyFrame::OnSetTextColour)
    EVT_MENU(ID_CUSTOM, MyFrame::OnSetStyle)
    EVT_MENU(ID_RECTANGLE, MyFrame::OnSetStyle)
    EVT_MENU(ID_ELIPSE, MyFrame::OnSetStyle)
    EVT_MENU(ID_TRIANGLE, MyFrame::OnSetStyle)
    EVT_MENU(ID_DIAMOND, MyFrame::OnSetStyle)
    EVT_MENU(ID_LINE, MyFrame::OnSetLineStyle)
    EVT_MENU(ID_ARROW, MyFrame::OnSetLineStyle)
    EVT_MENU(ID_SETBORDERTHCKNESS, MyFrame::OnSetBorderThickness)
    EVT_MENU(ID_SETCORNERRADIUS, MyFrame::OnSetCornerRadius)

    EVT_MENU(wxID_HELP, MyFrame::OnHelp)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)

    EVT_GRAPHTREE_DROP(wxID_ANY, MyFrame::OnGraphTreeDrop)
    EVT_TREE_ITEM_ACTIVATED(wxID_ANY, MyFrame::OnTreeItemActivated)

    EVT_GRAPH_NODE_ADD(MyFrame::OnAddNode)
    EVT_GRAPH_NODE_DELETE(MyFrame::OnDeleteNode)

    EVT_GRAPH_EDGE_ADD(MyFrame::OnAddEdge)
    EVT_GRAPH_EDGE_DELETE(MyFrame::OnDeleteEdge)

    EVT_GRAPH_CONNECT_FEEDBACK(MyFrame::OnConnectFeedback)
    EVT_GRAPH_CONNECT(MyFrame::OnConnect)

    EVT_GRAPH_NODE_CLICK(wxID_ANY, MyFrame::OnClickNode)
    EVT_GRAPH_NODE_ACTIVATE(wxID_ANY, MyFrame::OnActivateNode)
    EVT_GRAPH_NODE_MENU(wxID_ANY, MyFrame::OnMenuNode)
    EVT_GRAPH_EDGE_CLICK(wxID_ANY, MyFrame::OnClickEdge)
    EVT_GRAPH_EDGE_ACTIVATE(wxID_ANY, MyFrame::OnActivateEdge)
    EVT_GRAPH_EDGE_MENU(wxID_ANY, MyFrame::OnMenuEdge)
END_EVENT_TABLE()

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP(MyApp)

// ============================================================================
// impl
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
    wxInitAllImageHandlers();

    // create the main application window
    MyFrame *frame = new MyFrame(_T("GraphTest Example"));

    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
    frame->Show(true);

    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}

int MyApp::OnExit()
{
    return wxApp::OnExit();
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MyFrame::MyFrame(const wxString& title)
  : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1024, 768)),
    m_element(NULL),
    m_node(NULL)
{
    // set the frame icon
    wxIcon icon = wxICON(graphtest);
    SetIcon(icon);

    // file menu
    wxMenu *fileMenu = new wxMenu;
    fileMenu->Append(wxID_NEW);
    fileMenu->Append(wxID_OPEN);
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_SAVE);
    fileMenu->Append(wxID_SAVEAS);
    fileMenu->Append(ID_SAVEIMAGE, _T("Save &Image..."));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_PRINT);
    fileMenu->Append(wxID_PREVIEW);
    fileMenu->Append(wxID_PRINT_SETUP, _T("Print Set&up..."));
    fileMenu->Append(ID_PRINT_SCALING, _T("Print S&caling..."));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT);

    // edit menu
    wxMenu *editMenu = new wxMenu;
    editMenu->Append(wxID_UNDO)->Enable(false);
    editMenu->Append(wxID_REDO)->Enable(false);
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT)->Enable(false);
    editMenu->Append(wxID_COPY)->Enable(false);
    editMenu->Append(wxID_PASTE)->Enable(false);
    editMenu->Append(wxID_CLEAR, _T("&Delete\tDel"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_SELECTALL);

    // test menu
    wxMenu *testMenu = new wxMenu;
    testMenu->Append(ID_LAYOUTALL, _T("&Layout All\tCtrl+L"));
    testMenu->AppendSeparator();
    testMenu->AppendCheckItem(ID_SHOWGRID, _T("&Show &Grid\tCtrl+G"))->Check();
    testMenu->AppendCheckItem(ID_SNAPTOGRID, _T("Snap &To Grid\tCtrl+T"))->Check();
    testMenu->Append(ID_SETGRID, _T("Set Grid &Spacing..."));
    testMenu->AppendSeparator();
    testMenu->Append(wxID_ZOOM_IN, _T("Zoom&In\t+"));
    testMenu->Append(wxID_ZOOM_OUT, _T("Zoom&Out\t-"));
    testMenu->Append(ID_ZOOM, _T("Zoom\tAlt+Z"));

    // help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(wxID_HELP);
    helpMenu->AppendSeparator();
    helpMenu->Append(wxID_ABOUT);

    // menu bar
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(editMenu, _T("&Edit"));
    menuBar->Append(testMenu, _T("&Test"));
    menuBar->Append(helpMenu, _T("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    // Example of creating the graph control and graph.
    // Call Graph::SetEventHandler to receive events from the graph.
    wxSplitterWindow *splitter = new wxSplitterWindow(this);
    m_tree = new GraphTreeCtrl(splitter);
    m_graphctrl = new ProjectDesigner(splitter);
    m_graph = new Graph;
    m_graph->SetEventHandler(this);
    m_graphctrl->SetGraph(m_graph);
    splitter->SplitVertically(m_tree, m_graphctrl, 240);

    // grey grid on a white background
    m_graphctrl->SetForegroundColour(*wxLIGHT_GREY);
    m_graphctrl->SetBackgroundColour(*wxWHITE);

    enum { Icon_Normal };
    wxImageList *images = new wxImageList(16, 16);
    images->Add(icon);
    m_tree->AssignImageList(images);

    // populate the tree control
    wxTreeItemId id, idRoot = m_tree->AddRoot(_T("Root"));

    id = AppendTreeItem(idRoot, ImportNode());
    AppendTreeItem<ImportFileNode>(id);
    AppendTreeItem<ImportODBCNode>(id);
    m_tree->Expand(id);

    id = AppendTreeItem(idRoot, ExportNode());
    AppendTreeItem<ExportFileNode>(id);
    AppendTreeItem<ExportODBCNode>(id);
    m_tree->Expand(id);

    id = AppendTreeItem(idRoot, AnalyseNode());
    AppendTreeItem<SearchNode>(id);
    AppendTreeItem<SampleNode>(id);
    AppendTreeItem<SortNode>(id);
    AppendTreeItem<ValidateNode>(id);
    AppendTreeItem<AddressValNode>(id);
    m_tree->Expand(id);

    id = AppendTreeItem(idRoot, ReEngNode());
    AppendTreeItem<CleanNode>(id);
    AppendTreeItem<ExtractNode>(id);
    AppendTreeItem<SplitNode>(id);
    AppendTreeItem<UniteNode>(id);
    AppendTreeItem<InsertNode>(id);
    AppendTreeItem<DeleteNode>(id);
    AppendTreeItem<ArrangeNode>(id);
    AppendTreeItem<AppendNode>(id);
    AppendTreeItem<SQLQueryNode>(id);
    m_tree->Expand(id);

    id = AppendTreeItem(idRoot, MatchUpNode());
    AppendTreeItem<MatchNode>(id);
    AppendTreeItem<MatchTableNode>(id);
    AppendTreeItem<MergeNode>(id);
    m_tree->Expand(id);

    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText(_T("Welcome to GraphTest!"));

    m_pageSetupDialogData.SetMarginTopLeft(wxPoint(15, 15));
    m_pageSetupDialogData.SetMarginBottomRight(wxPoint(15, 15));

    m_printDialogData.GetPrintData().SetOrientation(wxLANDSCAPE);

    m_printscale = 100;
    m_pages = MaxPages(1, 1);
}

MyFrame::~MyFrame()
{
    delete m_graphctrl;
    delete m_graph;
}

template <class T> void MyFrame::AppendTreeItem(const wxTreeItemId& id)
{
    Factory<T> factory;
    const ProjectNode& node = factory.GetDefault();
    AppendTreeItem(id, node, new TreeItemData(factory));
}

wxTreeItemId MyFrame::AppendTreeItem(const wxTreeItemId& id,
                                     const ProjectNode& node,
                                     TreeItemData *tid)
{
    wxImageList *imglist = m_tree->GetImageList();
    int imgnum = imglist->Add(node.GetIcon());
    return m_tree->AppendItem(id, node.GetText(), imgnum, -1, tid);
}

ProjectNode *MyFrame::NewNode(const wxTreeItemId& id, const wxPoint& pt)
{
    TreeItemData *tid = static_cast<TreeItemData*>(m_tree->GetItemData(id));
    ProjectNode *node = NULL;

    if (tid) {
        node = tid->New();
        node->SetResult(_T("this is a multi-\nline test"));
        m_graph->Add(node, pt);
    }

    return node;
}

// event handlers

// Example of handling a drop event from the tree control and using
// Graph::Add to add node.
//
void MyFrame::OnGraphTreeDrop(GraphTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    NewNode(id, event.GetPosition());
}

// Tree item double clicked. Insert a node, searching for clear space to put
// it in.
//
void MyFrame::OnTreeItemActivated(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    wxPoint pt = m_graph->FindSpace(wxSize(200, 150));
    ProjectNode *node = NewNode(id, pt);

    if (node) {
        m_graph->UnselectAll();
        node->Select();
        m_graphctrl->EnsureVisible(*node);
    }
}

void MyFrame::OnAddNode(GraphEvent&)
{
    wxLogDebug(_T("OnAddNode"));
}

void MyFrame::OnDeleteNode(GraphEvent&)
{
    wxLogDebug(_T("OnDeleteNode"));
}

void MyFrame::OnClickNode(GraphEvent&)
{
    wxLogDebug(_T("OnClickNode"));
}

// An example of how to tell which part of a node has been clicked using
// the HitTest() method.
//
void MyFrame::OnActivateNode(GraphEvent& event)
{
    wxLogDebug(_T("OnActivateNode"));

    ProjectNode *node = wxStaticCast(event.GetNode(), ProjectNode);
    int hit = node->HitTest(event.GetPosition());

    if (hit == ProjectNode::Hit_Operation) {
        wxString str = TextPrompt(_T("Operation"), node->GetOperation());
        if (!str.empty())
            node->SetOperation(str);
    }
    else if (hit == ProjectNode::Hit_Result) {
        wxString str = TextPrompt(_T("Result"), node->GetResult());
        if (!str.empty())
            node->SetResult(str);
    }
}

wxString MyFrame::TextPrompt(const wxString& prompt, const wxString& value)
{
    wxString str = value;
    str.Replace(_T("\n"), _T("\\n"));
    str = wxGetTextFromUser(prompt + _T(":"), prompt, str, this);
    str.Replace(_T("\\n"), _T("\n"));
    return str;
}

// Example of handling a right click on a node to display a popup menu.
// The position returned by the event is in graph coordinates and can be
// converted to screen coordinates using GraphToScreen()
//
void MyFrame::OnMenuNode(GraphEvent& event)
{
    wxLogDebug(_T("OnMenuNode"));

    wxMenu menu;
    menu.Append(ID_LAYOUT, _T("&Layout Selection"));
    menu.Append(ID_SETSIZE, _T("Set &Size..."));
    menu.Append(ID_SETFONT, _T("Set &Font..."));
    menu.Append(ID_SETCOLOUR, _T("Set &Colour..."));
    menu.Append(ID_SETBGCOLOUR, _T("Set &Background Colour..."));
    menu.Append(ID_SETTEXTCOLOUR, _T("Set &Text Colour..."));
    menu.Append(ID_SETBORDERTHCKNESS, _T("Set Bor&der Thickness..."));
    menu.Append(ID_SETCORNERRADIUS, _T("Set Corner &Radius..."));

    wxMenu *submenu = new wxMenu;
    submenu->Append(ID_CUSTOM, _T("&Custom"));
    submenu->Append(ID_RECTANGLE, _T("&Rectangle"));
    submenu->Append(ID_ELIPSE, _T("&Elipse"));
    submenu->Append(ID_TRIANGLE, _T("&Triangle"));
    submenu->Append(ID_DIAMOND, _T("&Diamond"));
    menu.Append(ID_STYLE, _T("Set St&yle"), submenu);

    wxPoint pt = event.GetPosition();
    wxPoint ptClient = ScreenToClient(m_graphctrl->GraphToScreen(pt));

    m_element = m_node = event.GetNode();
    PopupMenu(&menu, ptClient.x, ptClient.y);
    m_element = m_node = NULL;
}

void MyFrame::OnAddEdge(GraphEvent&)
{
    wxLogDebug(_T("OnAddEdge"));
}

void MyFrame::OnDeleteEdge(GraphEvent&)
{
    wxLogDebug(_T("OnDeleteEdge"));
}

// This event fires during node dragging each time the cursor hovers over
// a potential target node, and allows the application to decide whether
// dropping here would create a link.
//
// GetSources() returns a list of source nodes, and GetTarget() returns the
// target node.  Removing nodes from the sources list disallows just that
// connection while permitting other sources to connect. Vetoing the event
// disallows all connections (it's equivalent to clearing the list).
//
// In this example no outgoing connections are allowed from Export nodes, and
// no incoming connections are allowed to Import nodes.
//
void MyFrame::OnConnectFeedback(GraphEvent& event)
{
    wxLogDebug(_T("OnConnectFeedback"));

    // Veto to disallow all connections
    wxString dest = event.GetTarget()->GetText();
    if (dest.find(_T("Import")) != wxString::npos)
        return event.Veto();

    GraphEvent::NodeList& sources = event.GetSources();
    GraphEvent::NodeList::iterator i = sources.begin(), j;

    // Remove from sources list to disallow only some connections
    while (i != sources.end()) {
        j = i++;
        if ((*j)->GetText().find(_T("Export")) != wxString::npos)
            sources.erase(j);
    }
}

// This event fires when nodes have been dropped on a target node. It is
// similar to OnConnectFeedback above. Vetoing the event disallows all
// connections or removing nodes from the list disallows just selected
// connections.
//
// In this example 'Sort' nodes are restricted to 1 input and 1 output
//
void MyFrame::OnConnect(GraphEvent& event)
{
    wxLogDebug(_T("OnConnect"));

    GraphNode *target = event.GetTarget();
    wxString dest = target->GetText();
    GraphEvent::NodeList& sources = event.GetSources();
    bool ok = true;

    // Veto to disallow all connections
    if (dest.find(_T("Sort")) != wxString::npos) {
        if (sources.size() + target->GetInEdgeCount() > 1) {
            event.Veto();
            ok = false;
        }
    }

    GraphEvent::NodeList::iterator i = sources.begin(), j;

    // Remove from sources list to disallow only some connections
    while (i != sources.end())
    {
        j = i++;

        wxString operation = (*j)->GetText();

        // output the operation text so that the order of the list is visible
        wxLogDebug(_T("    ") + operation);

        if (operation.find(_T("Sort")) != wxString::npos) {
            if ((*j)->GetOutEdgeCount() > 0) {
                sources.erase(j);
                ok = false;
            }
        }
    }

    if (!ok)
        wxLogError(_T("A 'Sort' node can have only one input and one output"));
}

void MyFrame::OnClickEdge(GraphEvent&)
{
    wxLogDebug(_T("OnClickEdge"));
}

void MyFrame::OnActivateEdge(GraphEvent&)
{
    wxLogDebug(_T("OnActivateEdge"));
}

void MyFrame::OnMenuEdge(GraphEvent& event)
{
    wxLogDebug(_T("OnMenuEdge"));

    wxMenu menu;
    menu.Append(ID_LAYOUT, _T("&Layout Selection"));
    menu.Append(ID_SETCOLOUR, _T("Set &Colour..."));
    menu.Append(ID_SETBGCOLOUR, _T("Set &Background Colour..."));

    wxMenu *submenu = new wxMenu;
    submenu->Append(ID_LINE, _T("&Line"));
    submenu->Append(ID_ARROW, _T("&Arrow"));
    menu.Append(ID_STYLE, _T("Set &Style"), submenu);

    wxPoint pt = event.GetPosition();
    wxPoint ptClient = ScreenToClient(m_graphctrl->GraphToScreen(pt));

    m_element = m_edge = event.GetEdge();
    PopupMenu(&menu, ptClient.x, ptClient.y);
    m_element = m_edge = NULL;
}

void MyFrame::OnQuit(wxCommandEvent&)
{
    // true is to force the frame to close
    Close(true);
}

void MyFrame::OnHelp(wxCommandEvent&)
{
    wxString name = _T("graphtest_help");
    wxWindow *win = FindWindow(name);

    if (win != NULL) {
        win->Raise();
    }
    else {
        wxFrame *frame = new wxFrame(this, wxID_ANY, _T("GraphTest Help"),
                                     wxDefaultPosition, wxSize(800, 600),
                                     wxDEFAULT_FRAME_STYLE, name);

        wxLogNull nolog;
        wxHtmlWindow *html = new wxHtmlWindow(frame);

        // work around the wxhtml bug in wxWidgets 2.6.3
#if defined __WXGTK__ && !wxUSE_UNICODE
        html->GetParser()->SetInputEncoding(wxFONTENCODING_SYSTEM);
#endif

        html->SetPage(helptext);

        frame->CentreOnParent();
        frame->Show();
    }
}

void MyFrame::OnAbout(wxCommandEvent&)
{
    wxMessageBox(_T("Example program for the graph editor\n\n"),
                 _T("About the GraphTest sample"),
                 wxOK | wxICON_INFORMATION,
                 this);
}

void MyFrame::OnSetSize(wxCommandEvent&)
{
    wxString str;
    wxSize size = m_element->GetSize<Points>();

    str << size.x << _T(", ") << size.y;

    str = wxGetTextFromUser(_T("Enter a new size for the selected nodes:"),
                            _T("Set Size"), str, this);

    if (!str.empty()) {
        wxChar sep = _T(',');
        size.x = wxAtoi(str.BeforeFirst(sep));
        size.y = wxAtoi(str.AfterFirst(sep));

        Graph::node_iterator it, end;

        for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it)
            it->SetSize<Points>(size);
    }
}

void MyFrame::OnSetFont(wxCommandEvent&)
{
    wxFont font = wxGetFontFromUser(this, m_node->GetFont());

    if (font.Ok()) {
        Graph::node_iterator it, end;

        for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it)
            it->SetFont(font);
    }
}

void MyFrame::OnSetColour(wxCommandEvent&)
{
    wxColour colour = wxGetColourFromUser(this, m_element->GetColour());

    if (colour.Ok()) {
        Graph::iterator it, end;

        for (tie(it, end) = m_graph->GetSelection(); it != end; ++it)
            it->SetColour(colour);
    }
}

void MyFrame::OnSetBgColour(wxCommandEvent&)
{
    wxColour colour = m_element->GetBackgroundColour();
    colour = wxGetColourFromUser(this, colour);

    if (colour.Ok()) {
        Graph::iterator it, end;

        for (tie(it, end) = m_graph->GetSelection(); it != end; ++it)
            it->SetBackgroundColour(colour);
    }
}

void MyFrame::OnSetTextColour(wxCommandEvent&)
{
    wxColour colour = wxGetColourFromUser(this, m_node->GetTextColour());

    if (colour.Ok()) {
        Graph::node_iterator it, end;

        for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it)
            it->SetTextColour(colour);
    }
}

void MyFrame::OnSetStyle(wxCommandEvent& event)
{
    Graph::node_iterator it, end;
    tie(it, end) = m_graph->GetSelectionNodes();

    while (it != end) {
        GraphNode& node= *it;
        ++it;
        node.SetStyle(event.GetId() - ID_CUSTOM + GraphNode::Style_Custom);
    }
}

void MyFrame::OnSetLineStyle(wxCommandEvent& event)
{
    Graph::iterator it, end;
    tie(it, end) = m_graph->GetSelection();

    while (it != end) {
        GraphEdge *edge = wxDynamicCast(&*it, GraphEdge);
        ++it;
        if (edge)
            edge->SetStyle(event.GetId() - ID_LINE + GraphEdge::Style_Line);
    }
}

void MyFrame::OnSetBorderThickness(wxCommandEvent&)
{
    ProjectNode *node = wxDynamicCast(m_node, ProjectNode);
    long thickness = node ? node->GetBorderThickness<Points>() : 0;

    thickness = wxGetNumberFromUser(
        _T("Enter a new thickness for the selected nodes' borders:"), _T(""),
        _T("Set Border Thickness"), thickness, 1, 100, this);

    if (thickness >= 1) {
        Graph::node_iterator it, end;

        for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it) {
            node = wxDynamicCast(&*it, ProjectNode);
            if (node)
                node->SetBorderThickness<Points>(thickness);
        }
    }
}

void MyFrame::OnSetCornerRadius(wxCommandEvent&)
{
    ProjectNode *node = wxDynamicCast(m_node, ProjectNode);
    long radius = node ? node->GetCornerRadius<Points>() : 0;

    radius = wxGetNumberFromUser(
        _T("Enter a new radius for the selected nodes' corners:"), _T(""),
        _T("Set Corner Radius"), radius, 1, 100, this);

    if (radius >= 1) {
        Graph::node_iterator it, end;

        for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it) {
            node = wxDynamicCast(&*it, ProjectNode);
            if (node)
                node->SetCornerRadius<Points>(radius);
        }
    }
}

void MyFrame::OnNew(wxCommandEvent&)
{
    m_graphctrl->SetGraph(NULL);
    delete m_graph;
    m_graph = new Graph;
    m_graphctrl->SetGraph(m_graph);
    m_graph->SetEventHandler(this);
    m_filename.clear();
}

bool MyFrame::PickFile(int flags)
{
    wxString filename = wxFileSelector(
        _T("Choose a filename"),
        wxEmptyString,
        wxEmptyString,
        _T("xml"),
        _T("Project Designer files (*.xml)|*.xml|All files (*.*)|*.*"),
        flags,
        this);

    if (filename.empty())
        return false;

    m_filename = filename;
    return true;
}

void MyFrame::OnOpen(wxCommandEvent&)
{
    if (!PickFile(wxFD_OPEN))
        return;

    wxFFileInputStream stream(m_filename);
    if (stream.IsOk())
        m_graph->Deserialise(stream);
}

void MyFrame::OnSave(wxCommandEvent& event)
{
    if (event.GetId() == wxID_SAVEAS || m_filename.empty())
        if (!PickFile(wxFD_SAVE | wxFD_OVERWRITE_PROMPT))
            return;

    wxFFileOutputStream stream(m_filename);
    if (stream.IsOk())
        m_graph->Serialise(stream);
}

void MyFrame::OnSaveImage(wxCommandEvent&)
{
    wxList& handlers = wxImage::GetHandlers();
    wxList::const_iterator it;
    wxString filter;
    std::vector<wxBitmapType> types;
    int index = 0;

    for (it = handlers.begin(); it != handlers.end(); ++it) {
        wxImageHandler *h = static_cast<wxImageHandler*>(*it);
        types.push_back(wxBitmapType(h->GetType()));
        if (!filter.empty())
            filter << _T("|");
        filter << h->GetName() << _T(" (*.")
              << h->GetExtension() << _T(")|*.")
              << h->GetExtension();
    }

    wxString filename = wxFileSelectorEx(
        _T("Choose a filename"),
        wxEmptyString,
        wxEmptyString,
        &index,
        filter,
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
        this);

    if (filename.empty())
        return;

    wxRect rc = m_graph->GetBounds();
    int w = rc.GetWidth(), h = rc.GetHeight(), b = 0;
    wxString str;
    str << w << _T(", ") << h << _T(", ") << b;

    str = wxGetTextFromUser(
        _T("Enter a size in pixels for the output image 'height, width, border'."),
        _T("Image Size"), str, this);

    wxSscanf(str.c_str(), _T("%d , %d , %d"), &w, &h, &b);
    if (w < 0 || h < 0 || b < 0 || w + h + b == 0)
        return;

    wxBitmap bmp(w + 2 * b, h + 2 * b, 32);
    {
        wxMemoryDC dc(bmp);

        // Draw doesn't clear the background
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();

        // this is needed to position the graph's bounding box in the bitmap
        dc.SetLogicalOrigin(rc.x, rc.y);

        // this is optional, it offsets the image 'b' pixels to leave a border
        dc.SetDeviceOrigin(b, b);

        // this is optional too, it scales the image to the size w x h pixels
        if (!rc.IsEmpty())
            dc.SetUserScale(double(w) / rc.GetWidth(),
                            double(h) / rc.GetHeight());

        m_graph->Draw(&dc);
    }
    bmp.SaveFile(filename, types[index]);
}

// Print the graph
//
void MyFrame::OnPrint(wxCommandEvent&)
{
    GraphPrintout printout(
        m_graph, m_pageSetupDialogData, m_printscale, m_pages);
    wxPrinter printer(&m_printDialogData);
    printer.Print(this, &printout);
}

// Show the page setup dialog
//
// The page setup dialog has some settings in common with the print dialog.
// So we copy the common subset from the print dialog's setting before showing
// the page setup, then copy it back afterwards.
//
// On Windows copying the common subset crashes the program, however there is
// a workaround, calling IsOk() first makes the problem go away.
//
void MyFrame::OnPrintSetup(wxCommandEvent&)
{
    // copy the common subset from the print dialog
    if (m_printDialogData.GetPrintData().IsOk())
        m_pageSetupDialogData = m_printDialogData.GetPrintData();

    // construct the dialog
    wxPageSetupDialog dlg(this, &m_pageSetupDialogData);

    // show it
    if (dlg.ShowModal() == wxID_OK) {
        // save any changes
        m_pageSetupDialogData = dlg.GetPageSetupDialogData();

        // copy back the common subset
        if (m_pageSetupDialogData.GetPrintData().IsOk())
            m_printDialogData = m_pageSetupDialogData.GetPrintData();
    }
}

// Print Preview
//
void MyFrame::OnPreview(wxCommandEvent&)
{
    GraphPrintout *printout1 = new GraphPrintout(
        m_graph, m_pageSetupDialogData, m_printscale, m_pages);
    GraphPrintout *printout2 = new GraphPrintout(
        m_graph, m_pageSetupDialogData, m_printscale, m_pages);

    // pass two printout objects: 1 for preview, and 2 for possible printing
    wxPrintPreview *preview = new wxPrintPreview(
        printout1, printout2, &m_printDialogData);
    if (!preview->Ok()) {
        delete preview;
        wxMessageBox(_("A printer needs to be installed for print preview."));
        return;
    }

    wxSize size = GetSize();
    size.x = size.x * 9 / 10;
    size.y = size.x * 3 / 4;

    wxPreviewFrame *frame = new wxPreviewFrame(
            preview,
            this,
            _("Print Preview"),
            wxDefaultPosition,
            size,
            wxDEFAULT_FRAME_STYLE | wxMAXIMIZE);

    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
}

void MyFrame::OnPrintScaling(wxCommandEvent&)
{
    wxString str;
    str << m_printscale << _T("%");
    if (m_pages.total)
        str << _T(", ") << m_pages.total;
    else if (m_pages.rows || m_pages.cols)
        str << _T(", ") << m_pages.rows << _T("x") << m_pages.cols;

    int MinScale = 5;
    int MaxScale = 1000;

    wxString prompt;
    prompt << _T("Enter:\n")
           << _T("    a max scaling percentage e.g. '100.0%' [") << MinScale << _T("-") << MaxScale << _T(")\n")
           << _T("and optionally one of:\n")
           << _T("    a max number of pages in rows and columns, e.g. '3x3', (0 = no limit)\n")
           << _T("or:\n")
           << _T("    a max total pages, e.g. '6' (not implemented)");

    str = wxGetTextFromUser(prompt, _T("Print Scaling"), str, this);

    str.MakeLower();
    str.Replace(_T(","), _T(" "));
    str.Replace(_T("%"), _T(" "));
    str.Replace(_T("x"), _T(" "));

    int r, c;

    switch (wxSscanf(str.c_str(), _T(" %lg %d %d"), &m_printscale, &r, &c)) {
        case 1:
            m_pages = MaxPages::Unlimited;
            break;
        case 2:
            m_pages = r;
            break;
        case 3:
            m_pages = MaxPages(r, c);
            break;
    }

    m_printscale = wxMin(MaxScale, wxMax(MinScale, m_printscale));
}

void MyFrame::OnCut(wxCommandEvent&)
{
    m_graph->Cut();
}

void MyFrame::OnCopy(wxCommandEvent&)
{
    m_graph->Copy();
}

void MyFrame::OnPaste(wxCommandEvent&)
{
    m_graph->Paste();
}

void MyFrame::OnClear(wxCommandEvent&)
{
    m_graph->Clear();
}

void MyFrame::OnSelectAll(wxCommandEvent&)
{
    m_graph->SelectAll();
}

void MyFrame::OnLayout(wxCommandEvent&)
{
    m_graph->Layout(m_graph->GetSelectionNodes());
}

void MyFrame::OnLayoutAll(wxCommandEvent&)
{
    m_graph->LayoutAll();
}

void MyFrame::OnZoomIn(wxCommandEvent&)
{
    double zoom = m_graphctrl->GetZoom();
    zoom += ZoomStep;
    if (zoom <= ZoomMax)
        m_graphctrl->SetZoom(zoom);
}

void MyFrame::OnZoomOut(wxCommandEvent&)
{
    double zoom = m_graphctrl->GetZoom();
    zoom -= ZoomStep;
    if (zoom >= ZoomMin)
        m_graphctrl->SetZoom(zoom);
}

void MyFrame::OnSetZoom(wxCommandEvent&)
{
    wxString prompt;
    prompt << _T("Enter a new zoom percentage [");
    prompt << ZoomMin << _T(".0-") << ZoomMax << _T(".0]:");

    wxString str;
    str << m_graphctrl->GetZoom();

    str = wxGetTextFromUser(prompt, _T("Set Zoom"), str, this);

    double zoom;
    if (str.ToDouble(&zoom))
        m_graphctrl->SetZoom(wxMin(ZoomMax, wxMax(ZoomMin, zoom)));
}

void MyFrame::OnShowGrid(wxCommandEvent&)
{
    m_graphctrl->SetShowGrid(!m_graphctrl->IsGridShown());
}

void MyFrame::OnUIShowGrid(wxUpdateUIEvent& event)
{
    event.Check(m_graphctrl->IsGridShown());
}

void MyFrame::OnSnapToGrid(wxCommandEvent&)
{
    m_graph->SetSnapToGrid(!m_graph->GetSnapToGrid());
}

void MyFrame::OnUISnapToGrid(wxUpdateUIEvent& event)
{
    event.Check(m_graph->GetSnapToGrid());
}

void MyFrame::OnSetGrid(wxCommandEvent&)
{
    long spacing = wxGetNumberFromUser(_T(""), _T("Grid Spacing:"),
                                       _T("Grid Spacing"),
                                       m_graph->GetGridSpacing<Points>(),
                                       1, 100, this);
    if (spacing >= 1)
        m_graph->SetGridSpacing<Points>(spacing);
}
