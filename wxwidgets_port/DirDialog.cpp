#include "DirDialog.h"
#include <wx/dirdlg.h>
#include <wx/button.h>
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(DirDialog, wxDialog)
    EVT_BUTTON(wxID_OK, DirDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, DirDialog::OnCancel)
wxEND_EVENT_TABLE()

DirDialog::DirDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Select Directory"), wxDefaultPosition, wxSize(400, 500))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    m_dirCtrl = new wxGenericDirCtrl(this, wxID_ANY, wxT(""), 
                                      wxDefaultPosition, wxDefaultSize,
                                      wxDIRCTRL_DIR_ONLY | wxDIRCTRL_3D_INTERNAL);
    
    mainSizer->Add(m_dirCtrl, 1, wxEXPAND | wxALL, 5);
    
    wxStdDialogButtonSizer* btnSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    Centre();
}

DirDialog::~DirDialog()
{
}

void DirDialog::SetInitialDirectory(const wxString& dir)
{
    if (!dir.IsEmpty() && wxDirExists(dir))
        m_dirCtrl->SetPath(dir);
}

wxString DirDialog::GetDirectory() const
{
    return m_dirCtrl->GetPath();
}

void DirDialog::OnOK(wxCommandEvent& event)
{
    EndModal(wxID_OK);
}

void DirDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

