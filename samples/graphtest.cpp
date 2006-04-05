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

#include "graphtree.h"
#include "projectdesigner.h"

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources and even
// though we could still include the XPM here it would be unused)
#if !defined(__WXMSW__) && !defined(__WXPM__)
    #include "graphtest.xpm"
#endif

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

using datactics::ProjectDesigner;
using datactics::ProjectNode;
using tt_solutions::GraphTreeEvent;
using tt_solutions::GraphTreeCtrl;
using tt_solutions::GraphElement;
using tt_solutions::GraphNode;
using tt_solutions::GraphEvent;
using tt_solutions::Graph;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

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

    void OnGraphTreeDrop(GraphTreeEvent& event);

    void OnAddNode(GraphEvent& event);
    void OnDeleteNode(GraphEvent& event);

    void OnAddEdge(GraphEvent& event);
    void OnAddingEdge(GraphEvent& event);
    void OnDeleteEdge(GraphEvent& event);

    void OnClickNode(GraphEvent& event);
    void OnActivateNode(GraphEvent& event);
    void OnMenuNode(GraphEvent& event);

    void OnClickEdge(GraphEvent& event);
    void OnActivateEdge(GraphEvent& event);
    void OnMenuEdge(GraphEvent& event);

    // file menu
    void OnOpen(wxCommandEvent &event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
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
    void OnShowGrid(wxCommandEvent& event);
    void OnSetGrid(wxCommandEvent&);

    // help menu
    void OnAbout(wxCommandEvent& event);

    // context menu
    void OnSetSize(wxCommandEvent& event);
    void OnLayout(wxCommandEvent& event);
    void OnSetFont(wxCommandEvent& event);
    void OnSetColour(wxCommandEvent& event);
    void OnSetBgColour(wxCommandEvent& event);
    void OnSetTextColour(wxCommandEvent& event);

    wxString TextPrompt(const wxString& prompt, const wxString& value);

    enum {
        ZoomMin = 25,
        ZoomMax = 300,
        ZoomStep = 25
    };

private:
    ProjectDesigner *m_graphctrl;
    GraphElement *m_element;
    GraphNode *m_node;
    Graph *m_graph;

    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum {
    ID_LAYOUTALL,
    ID_SHOWGRID,
    ID_SETGRID,
    ID_LAYOUT,
    ID_SETSIZE,
    ID_SETFONT,
    ID_SETCOLOUR,
    ID_SETBGCOLOUR,
    ID_SETTEXTCOLOUR
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(wxID_OPEN, MyFrame::OnOpen)
    EVT_MENU(wxID_SAVE, MyFrame::OnSave)
    EVT_MENU(wxID_SAVEAS, MyFrame::OnSaveAs)

    EVT_MENU(wxID_CUT, MyFrame::OnCut)
    EVT_MENU(wxID_COPY, MyFrame::OnCopy)
    EVT_MENU(wxID_PASTE, MyFrame::OnPaste)
    EVT_MENU(wxID_CLEAR, MyFrame::OnClear)
    EVT_MENU(wxID_SELECTALL, MyFrame::OnSelectAll)

    EVT_MENU(ID_LAYOUTALL, MyFrame::OnLayoutAll)
    EVT_MENU(wxID_ZOOM_IN, MyFrame::OnZoomIn)
    EVT_MENU(wxID_ZOOM_OUT, MyFrame::OnZoomOut)
    EVT_MENU(ID_SHOWGRID, MyFrame::OnShowGrid)
    EVT_MENU(ID_SETGRID, MyFrame::OnSetGrid)

    EVT_MENU(ID_LAYOUT, MyFrame::OnLayout)
    EVT_MENU(ID_SETSIZE, MyFrame::OnSetSize)
    EVT_MENU(ID_SETFONT, MyFrame::OnSetFont)
    EVT_MENU(ID_SETCOLOUR, MyFrame::OnSetColour)
    EVT_MENU(ID_SETBGCOLOUR, MyFrame::OnSetBgColour)
    EVT_MENU(ID_SETTEXTCOLOUR, MyFrame::OnSetTextColour)

    EVT_GRAPHTREE_DROP(wxID_ANY, MyFrame::OnGraphTreeDrop)

    EVT_GRAPH_NODE_ADD(MyFrame::OnAddNode)
    EVT_GRAPH_NODE_DELETE(MyFrame::OnDeleteNode)

    EVT_GRAPH_EDGE_ADD(MyFrame::OnAddEdge)
    EVT_GRAPH_EDGE_ADDING(MyFrame::OnAddingEdge)
    EVT_GRAPH_EDGE_DELETE(MyFrame::OnDeleteEdge)

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
  : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
    m_element(NULL),
    m_node(NULL)
{
    // set the frame icon
    wxIcon icon = wxICON(graphtest);
    SetIcon(icon);

    // file menu
    wxMenu *fileMenu = new wxMenu;
    fileMenu->Append(wxID_OPEN, _T("&Open\tCtrl+O"), _T("Open a file"))->Enable(false);
    fileMenu->Append(wxID_SAVE, _T("&Save\tCtrl+S"), _T("Save a file"))->Enable(false);
    fileMenu->Append(wxID_SAVEAS, _T("&Save As...\tF12"), _T("Save to a new file"))->Enable(false);
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit this program"));

    // edit menu
    wxMenu *editMenu = new wxMenu;
    editMenu->Append(wxID_UNDO, _T("&Undo\tCtrl+Z"))->Enable(false);
    editMenu->Append(wxID_REDO, _T("&Redo\tCtrl+Y"))->Enable(false);
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT, _T("Cu&t\tCtrl+X"))->Enable(false);
    editMenu->Append(wxID_COPY, _T("&Copy\tCtrl+C"))->Enable(false);
    editMenu->Append(wxID_PASTE, _T("&Paste\tCtrl+V"))->Enable(false);
    editMenu->Append(wxID_CLEAR, _T("&Delete\tDel"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_SELECTALL, _T("Select &All\tCtrl+A"));

    // test menu
    wxMenu *testMenu = new wxMenu;
    testMenu->Append(ID_LAYOUTALL, _T("&Layout All\tCtrl+L"));
    testMenu->AppendSeparator();
    testMenu->AppendCheckItem(ID_SHOWGRID, _T("&Show Grid\tCtrl+G"))->Check();
    testMenu->Append(ID_SETGRID, _T("&Set Grid Spacing..."));

#ifndef __WXMSW__
    testMenu->AppendSeparator();
    testMenu->Append(wxID_ZOOM_IN, _T("Zoom&In\t+"));
    testMenu->Append(wxID_ZOOM_OUT, _T("Zoom&Out\t-"));
#endif

    // help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, _T("&About...\tF1"), _T("Show about dialog"));

    // menu bar
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(editMenu, _T("&Edit"));
    menuBar->Append(testMenu, _T("&Test"));
    menuBar->Append(helpMenu, _T("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    wxSplitterWindow *splitter = new wxSplitterWindow(this);
    GraphTreeCtrl *tree = new GraphTreeCtrl(splitter);
    m_graphctrl = new ProjectDesigner(splitter);
    m_graph = new Graph;
    m_graph->SetEventHandler(this);
    m_graphctrl->SetGraph(m_graph);
    splitter->SplitVertically(tree, m_graphctrl, 210);

    m_graphctrl->SetBackgroundGradient(*wxWHITE, wxColour(0x1f97f6));
    m_graphctrl->SetForegroundColour(*wxWHITE);

    wxImageList *images = new wxImageList(icon.GetWidth(), icon.GetHeight());
    images->Add(icon);
    tree->AssignImageList(images);

    wxTreeItemId id, idRoot = tree->AddRoot(_T("Root"));
    id = tree->AppendItem(idRoot, _T("Import"));
    tree->AppendItem(id, _T("Flat File Import"), 0, 0, NULL);
    tree->Expand(id);
    id = tree->AppendItem(idRoot, _T("Export"));
    tree->AppendItem(id, _T("File Export"), 0, 0, NULL);
    tree->Expand(id);

    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText(_T("Welcome to GraphTest!"));
}

MyFrame::~MyFrame()
{
    delete m_graphctrl;
    delete m_graph;
}

// event handlers

void MyFrame::OnGraphTreeDrop(GraphTreeEvent& event)
{
    ProjectNode *node = new ProjectNode;
    node->SetText(event.GetString());
    node->SetResult(_T("this is a multi-\nline test"));
    node->SetColour(0x16a8fa);
    node->SetIcon(event.GetIcon());
    event.GetTarget()->GetGraph()->Add(node, event.GetPosition());
}

void MyFrame::OnAddNode(GraphEvent& event)
{
    wxLogDebug(_T("OnAddNode"));
}

void MyFrame::OnDeleteNode(GraphEvent& event)
{
    wxLogDebug(_T("OnDeleteNode"));
}

void MyFrame::OnClickNode(GraphEvent& event)
{
    wxLogDebug(_T("OnClickNode"));
}

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

    wxPoint pt = event.GetPosition();
    wxPoint ptClient = ScreenToClient(m_graphctrl->GraphToScreen(pt));

    m_element = m_node = event.GetNode();
    PopupMenu(&menu, ptClient.x, ptClient.y);
    m_element = m_node = NULL;
}

void MyFrame::OnAddEdge(GraphEvent& event)
{
    wxLogDebug(_T("OnAddEdge"));
}

void MyFrame::OnAddingEdge(GraphEvent& event)
{
    wxLogDebug(_T("OnAddingEdge"));

    wxString src = event.GetNode()->GetText();
    wxString dest = event.GetTarget()->GetText();

    if (dest.find(_T("Import")) != wxString::npos
            || src.find(_T("Export")) != wxString::npos)
        event.Veto();
}

void MyFrame::OnDeleteEdge(GraphEvent& event)
{
    wxLogDebug(_T("OnDeleteEdge"));
}

void MyFrame::OnClickEdge(GraphEvent& event)
{
    wxLogDebug(_T("OnClickEdge"));
}

void MyFrame::OnActivateEdge(GraphEvent& event)
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

    wxPoint pt = event.GetPosition();
    wxPoint ptClient = ScreenToClient(m_graphctrl->GraphToScreen(pt));

    m_element = event.GetEdge();
    PopupMenu(&menu, ptClient.x, ptClient.y);
    m_element = NULL;
}

void MyFrame::OnQuit(wxCommandEvent&)
{
    // true is to force the frame to close
    Close(true);
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
    wxSize size = m_element->GetSize();

    str << size.x << _T(", ") << size.y;

    str = wxGetTextFromUser(_T("Enter a new size for the selected nodes:"),
                            _T("Set Size"), str, this);

    if (!str.empty()) {
        wxChar sep = _T(',');
        size.x = atoi(str.BeforeFirst(sep).mb_str());
        size.y = atoi(str.AfterFirst(sep).mb_str());

        Graph::node_iterator it, end;

        for (tie(it, end) = m_graph->GetSelectionNodes(); it != end; ++it)
            it->SetSize(size);
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

void MyFrame::OnOpen(wxCommandEvent&)
{
    wxString path;
    wxString filename;

    wxFileDialog dialog(this, _T("Choose a filename"), path, filename,
            _T("Project Designer files (*.des)|*.des|All files (*.*)|*.*"),
            wxOPEN);

    if (dialog.ShowModal() == wxID_OK)
    {
    }
}

void MyFrame::OnSave(wxCommandEvent& event)
{
    OnSaveAs(event);
}

void MyFrame::OnSaveAs(wxCommandEvent&)
{
    wxString path;
    wxString filename;

    wxFileDialog dialog(this, _T("Choose a filename"), path, filename,
                        _T("*.des"), wxSAVE);

    if (dialog.ShowModal() == wxID_OK)
    {
    }
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
    int zoom = m_graphctrl->GetZoom();
    zoom += ZoomStep;
    if (zoom <= ZoomMax)
        m_graphctrl->SetZoom(zoom);
}

void MyFrame::OnZoomOut(wxCommandEvent&)
{
    int zoom = m_graphctrl->GetZoom();
    zoom -= ZoomStep;
    if (zoom >= ZoomMin)
        m_graphctrl->SetZoom(zoom);
}

void MyFrame::OnShowGrid(wxCommandEvent&)
{
    m_graphctrl->SetShowGrid(!m_graphctrl->IsGridShown());
}

void MyFrame::OnSetGrid(wxCommandEvent&)
{
    long spacing = wxGetNumberFromUser(_T(""), _T("Grid Spacing:"),
                                       _T("Grid Spacing"),
                                       m_graph->GetGridSpacing(),
                                       1, 100, this);
    if (spacing >= 1)
        m_graph->SetGridSpacing(spacing);
}
