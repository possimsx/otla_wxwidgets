#include "ModifyDialog.h"
#include "../source/funciones.h"
#include <wx/msgdlg.h>
#include <cstring>

wxBEGIN_EVENT_TABLE(ModifyDialog, wxDialog)
    EVT_BUTTON(wxID_OK, ModifyDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, ModifyDialog::OnCancel)
wxEND_EVENT_TABLE()

ModifyDialog::ModifyDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Modify Block"), wxDefaultPosition, wxSize(400, 400))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 5, 5);
    grid->AddGrowableCol(1);
    
    grid->Add(new wxStaticText(this, wxID_ANY, wxT("Name:")), 0, wxALIGN_CENTER_VERTICAL);
    m_nameEdt = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(200, -1));
    grid->Add(m_nameEdt, 0, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, wxT("Start:")), 0, wxALIGN_CENTER_VERTICAL);
    m_startEdt = new wxTextCtrl(this, wxID_ANY);
    grid->Add(m_startEdt, 0, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, wxT("Size:")), 0, wxALIGN_CENTER_VERTICAL);
    m_sizeEdt = new wxTextCtrl(this, wxID_ANY);
    grid->Add(m_sizeEdt, 0, wxEXPAND);
    
    grid->Add(new wxStaticText(this, wxID_ANY, wxT("Exec:")), 0, wxALIGN_CENTER_VERTICAL);
    m_execEdt = new wxTextCtrl(this, wxID_ANY);
    grid->Add(m_execEdt, 0, wxEXPAND);
    
    mainSizer->Add(grid, 0, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Data (hex):")), 0, wxALL, 5);
    m_memo = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(-1, 150),
                            wxTE_MULTILINE | wxTE_READONLY);
    // Monospace font so hex dump columns align (matches the original
    // ModificaForm.dfm which sets Memo1.Font.Name = 'Terminal').
    m_memo->SetFont(wxFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE,
                           wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    mainSizer->Add(m_memo, 1, wxEXPAND | wxALL, 5);
    
    wxStdDialogButtonSizer* btnSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    Centre();
}

ModifyDialog::~ModifyDialog()
{
}

void ModifyDialog::SetBlock(ts_sbb_block* block)
{
    if (!block)
        return;
    
    // Ensure blockname is null-terminated
    char safeName[17];
    strncpy(safeName, block->blockname, 16);
    safeName[16] = '\0';
    
    m_nameEdt->SetValue(wxString(safeName, wxConvLocal));
    m_startEdt->SetValue(wxString::Format(wxT("%u"), block->ini));
    m_sizeEdt->SetValue(wxString::Format(wxT("%u"), block->size));
    m_execEdt->SetValue(wxString::Format(wxT("%u"), block->exec));
    
    // Display data as hex (limit to reasonable size)
    wxString hexData;
    unsigned int displaySize = block->size;
    if (displaySize > 0x10000) // Sanity check
        displaySize = 0x10000;
    if (displaySize > 256)
        displaySize = 256; // Only show first 256 bytes
    
    for (unsigned int i = 0; i < displaySize; i++)
    {
        if (i > 0 && (i % 16) == 0)
            hexData += wxT("\n");
        hexData += wxString::Format(wxT("%02X "), block->data[i]);
    }
    m_memo->SetValue(hexData);
}

void ModifyDialog::GetBlock(ts_sbb_block* block)
{
    if (!block)
        return;
    
    wxString name = m_nameEdt->GetValue();
    strncpy(block->blockname, name.mb_str(wxConvLocal), sizeof(block->blockname) - 1);
    block->blockname[sizeof(block->blockname) - 1] = '\0';
    
    wxString startStr = m_startEdt->GetValue();
    block->ini = inputDecHex(startStr.mb_str(wxConvLocal));
    
    wxString sizeStr = m_sizeEdt->GetValue();
    block->size = inputDecHex(sizeStr.mb_str(wxConvLocal));
    
    wxString execStr = m_execEdt->GetValue();
    block->exec = inputDecHex(execStr.mb_str(wxConvLocal));
}

void ModifyDialog::OnOK(wxCommandEvent& event)
{
    EndModal(wxID_OK);
}

void ModifyDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

