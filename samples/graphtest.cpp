/////////////////////////////////////////////////////////////////////////////
// Name:        graphtest.cpp
// Purpose:     Example program for the wxGraph graph editor
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
#include <graphtree.h>
#include <projectdesigner.h>

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// the application icon (under Windows and OS/2 it is in resources and even
// though we could still include the XPM here it would be unused)
#if !defined(__WXMSW__) && !defined(__WXPM__)
    #include "../sample.xpm"
#endif

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

using datactics::ProjectDesigner;
using datactics::ProjectNode;
using tt_solutions::GraphTreeEvent;
using tt_solutions::GraphTreeCtrl;
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
    void OnLayout(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);

    // help menu
    void OnAbout(wxCommandEvent& event);

    enum {
        ZoomMin = 25,
        ZoomMax = 300,
        ZoomStep = 25
    };

private:
    ProjectDesigner *m_graphctrl;
    Graph *m_graph;

    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum {
    ID_LAYOUT
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MyFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(wxID_OPEN, MyFrame::OnOpen)
    EVT_MENU(wxID_SAVE,  MyFrame::OnSave)
    EVT_MENU(wxID_SAVEAS,  MyFrame::OnSaveAs)
    EVT_MENU(wxID_CUT,  MyFrame::OnCut)
    EVT_MENU(wxID_COPY,  MyFrame::OnCopy)
    EVT_MENU(wxID_PASTE,  MyFrame::OnPaste)
    EVT_MENU(wxID_CLEAR,  MyFrame::OnClear)
    EVT_MENU(wxID_SELECTALL, MyFrame::OnSelectAll)
    EVT_MENU(ID_LAYOUT, MyFrame::OnLayout)
    EVT_MENU(wxID_ZOOM_IN, MyFrame::OnZoomIn)
    EVT_MENU(wxID_ZOOM_OUT, MyFrame::OnZoomOut)
    EVT_GRAPHTREE_DROP(wxID_ANY,  MyFrame::OnGraphTreeDrop)
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
    MyFrame *frame = new MyFrame(_T("wxGraph Example"));

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
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600))
{
    // set the frame icon
    wxIcon icon = wxICON(graphtest);
    SetIcon(icon);

    // file menu
    wxMenu *fileMenu = new wxMenu;
    fileMenu->Append(wxID_OPEN, _T("&Open\tCtrl+O"), _T("Open a file"));
    fileMenu->Append(wxID_SAVE, _T("&Save\tCtrl+S"), _T("Save a file"));
    fileMenu->Append(wxID_SAVEAS, _T("&Save As...\tF12"), _T("Save to a new file"));
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit this program"));

    // edit menu
    wxMenu *editMenu = new wxMenu;
    editMenu->Append(wxID_UNDO, _("&Undo\tCtrl+Z"));
    editMenu->Append(wxID_REDO, _("&Redo\tCtrl+Y"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT, _("Cu&t\tCtrl+X"));
    editMenu->Append(wxID_COPY, _("&Copy\tCtrl+C"));
    editMenu->Append(wxID_PASTE, _("&Paste\tCtrl+V"));
    editMenu->Append(wxID_CLEAR, _("&Delete\tDel"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_SELECTALL, _("Select &All\tCtrl+A"));
    editMenu->AppendSeparator();
    editMenu->Append(ID_LAYOUT, _("&Layout\tCtrl+L"));
    editMenu->AppendSeparator();
    editMenu->Append(wxID_ZOOM_IN, _("Zoom&In\t+"));
    editMenu->Append(wxID_ZOOM_OUT, _("Zoom&Out\t-"));

    // help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, _T("&About...\tF1"), _T("Show about dialog"));

    // menu bar
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(editMenu, _T("&Edit"));
    menuBar->Append(helpMenu, _T("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    wxSplitterWindow *splitter = new wxSplitterWindow(this);
    GraphTreeCtrl *tree = new GraphTreeCtrl(splitter);
    m_graphctrl = new ProjectDesigner(splitter);
    m_graph = new Graph;
    m_graphctrl->SetGraph(m_graph);
    splitter->SplitVertically(tree, m_graphctrl, 210);

    m_graphctrl->SetBackgroundColour(wxColour(0x1f97f6));
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
    SetStatusText(_T("Welcome to wxGraph!"));
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
    node->SetTextColour(0xfaa816);
    node->SetBackgroundColour(*wxCYAN);
    node->SetIcon(event.GetIcon());
    event.GetTarget()->GetGraph()->Add(node, event.GetPosition());
}

void MyFrame::OnQuit(wxCommandEvent&)
{
    // true is to force the frame to close
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent&)
{
    wxMessageBox(_T("Example program for the wxGraph graph editor\n\n"),
                 _T("About wxGraph sample"),
                 wxOK | wxICON_INFORMATION,
                 this);
}

void MyFrame::OnOpen(wxCommandEvent&)
{
    wxString path;
    wxString filename;

    wxFileDialog dialog(this, _("Choose a filename"), path, filename,
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

    wxFileDialog dialog(this, _("Choose a filename"), path, filename,
                        _T("*.des"), wxSAVE);

    if (dialog.ShowModal() == wxID_OK)
    {
    }
}

void MyFrame::OnCut(wxCommandEvent&)
{
    //m_graphctrl->GetGraph()->Cut();
}

void MyFrame::OnCopy(wxCommandEvent&)
{
    //m_graphctrl->GetGraph()->Copy();
}

void MyFrame::OnPaste(wxCommandEvent&)
{
    //m_graphctrl->GetGraph()->Paste();
}

void MyFrame::OnClear(wxCommandEvent&)
{
    m_graphctrl->GetGraph()->Clear();
}

void MyFrame::OnSelectAll(wxCommandEvent&)
{
    m_graphctrl->GetGraph()->SelectAll();
}

void MyFrame::OnLayout(wxCommandEvent&)
{
    m_graphctrl->GetGraph()->Layout();
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
