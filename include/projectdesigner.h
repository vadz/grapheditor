/////////////////////////////////////////////////////////////////////////////
// Name:        projectdesigner.h
// Purpose:     Classes for laying out project graphs
// Author:      Mike Wetherell
// Modified by:
// Created:     March 2006
// RCS-ID:      $Id$
// Copyright:   (c) 2006 TT-Solutions SARL
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef PROJECTDESIGNER_H
#define PROJECTDESIGNER_H

#include <graphctrl.h>

namespace datactics {

/**
 * @brief A custom GraphNode for the ProjectDesigner.
 *
 * @see ProjectDesigner
 */
class DesignerNode : public tt_solutions::GraphNode
{
public:
    /** @brief Constructor. */
    DesignerNode();
    /** @brief Destructor. */
    ~DesignerNode();

    /** @brief The node's id. */
    wxString GetId() const                  { return m_id; }
    /** @brief The node's operation label. */
    wxString GetOperation() const           { return GetText(); }
    /** @brief The node's result label. */
    wxString GetResult() const              { return m_result; }

    /** @brief The node's id. */
    void SetId(const wxString& text);
    /** @brief The node's operation label. */
    void SetOperation(const wxString& text) { return SetText(text); }
    /** @brief The node's result label. */
    void SetResult(const wxString& text);

    bool Serialize(wxOutputStream& out);
    bool Deserialize(wxInputStream& in);

private:
    wxString m_id;
    wxString m_result;
};

/**
 * @brief Graph layout control for Datactics projects.
 */
class ProjectDesigner : public tt_solutions::GraphCtrl,
                        public tt_solutions::Graph
{
public:
    /** @brief Constructor. */
    ProjectDesigner(wxWindow *parent,
                    wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = wxBORDER | wxRETAINED,
                    const wxString& name = DefaultName);

    /** @brief Destructor. */
    ~ProjectDesigner();

    static const wxChar DefaultName[];
};

} // namespace datactics

#endif // PROJECTDESIGNER_H
