#include "SettingsDialog.h"
#include "DirDialog.h"
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/sizer.h>
#include <wx/button.h>

// Event table - only for standard buttons
wxBEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, SettingsDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, SettingsDialog::OnCancel)
wxEND_EVENT_TABLE()

SettingsDialog::SettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Settings"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Builder group
    wxStaticBox* builderBox = new wxStaticBox(this, wxID_ANY, wxT("Builder"));
    wxStaticBoxSizer* builderSizer = new wxStaticBoxSizer(builderBox, wxVERTICAL);
    
    wxFlexGridSizer* builderGrid = new wxFlexGridSizer(2, 5, 5);
    builderGrid->AddGrowableCol(1);
    
    // ZX Directory
    builderGrid->Add(new wxStaticText(this, wxID_ANY, wxT("ZX Output:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* zxSizer = new wxBoxSizer(wxHORIZONTAL);
    m_dirZXEdt = new wxTextCtrl(this, wxID_ANY);
    wxButton* dirZXBtn = new wxButton(this, wxID_ANY, wxT("..."));
    dirZXBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnDirZXBtn, this);
    zxSizer->Add(m_dirZXEdt, 1, wxEXPAND);
    zxSizer->Add(dirZXBtn, 0, wxLEFT, 5);
    builderGrid->Add(zxSizer, 1, wxEXPAND);
    
    // CPC Directory
    builderGrid->Add(new wxStaticText(this, wxID_ANY, wxT("CPC Output:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* cpcSizer = new wxBoxSizer(wxHORIZONTAL);
    m_dirCPCEdt = new wxTextCtrl(this, wxID_ANY);
    wxButton* dirCPCBtn = new wxButton(this, wxID_ANY, wxT("..."));
    dirCPCBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnDirCPCBtn, this);
    cpcSizer->Add(m_dirCPCEdt, 1, wxEXPAND);
    cpcSizer->Add(dirCPCBtn, 0, wxLEFT, 5);
    builderGrid->Add(cpcSizer, 1, wxEXPAND);
    
    // MSX Directory
    builderGrid->Add(new wxStaticText(this, wxID_ANY, wxT("MSX Output:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* msxSizer = new wxBoxSizer(wxHORIZONTAL);
    m_dirMSXEdt = new wxTextCtrl(this, wxID_ANY);
    wxButton* dirMSXBtn = new wxButton(this, wxID_ANY, wxT("..."));
    dirMSXBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnDirMSXBtn, this);
    msxSizer->Add(m_dirMSXEdt, 1, wxEXPAND);
    msxSizer->Add(dirMSXBtn, 0, wxLEFT, 5);
    builderGrid->Add(msxSizer, 1, wxEXPAND);
    
    // ZX81 Directory
    builderGrid->Add(new wxStaticText(this, wxID_ANY, wxT("ZX81 Output:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* zx81Sizer = new wxBoxSizer(wxHORIZONTAL);
    m_dir81Edt = new wxTextCtrl(this, wxID_ANY);
    wxButton* dir81Btn = new wxButton(this, wxID_ANY, wxT("..."));
    dir81Btn->Bind(wxEVT_BUTTON, &SettingsDialog::OnDir81Btn, this);
    zx81Sizer->Add(m_dir81Edt, 1, wxEXPAND);
    zx81Sizer->Add(dir81Btn, 0, wxLEFT, 5);
    builderGrid->Add(zx81Sizer, 1, wxEXPAND);
    
    builderSizer->Add(builderGrid, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(builderSizer, 0, wxEXPAND | wxALL, 5);
    
    // Tools group
    wxStaticBox* toolsBox = new wxStaticBox(this, wxID_ANY, wxT("Tools"));
    wxStaticBoxSizer* toolsSizer = new wxStaticBoxSizer(toolsBox, wxVERTICAL);
    
    wxFlexGridSizer* toolsGrid = new wxFlexGridSizer(2, 5, 5);
    toolsGrid->AddGrowableCol(1);
    
    // Disk Manager
    toolsGrid->Add(new wxStaticText(this, wxID_ANY, wxT("MSX Disk Manager:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* dskmgrSizer = new wxBoxSizer(wxHORIZONTAL);
    m_diskMgrEdt = new wxTextCtrl(this, wxID_ANY);
    wxButton* dskmgrBtn = new wxButton(this, wxID_ANY, wxT("..."));
    dskmgrBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnDiskMgrBtn, this);
    dskmgrSizer->Add(m_diskMgrEdt, 1, wxEXPAND);
    dskmgrSizer->Add(dskmgrBtn, 0, wxLEFT, 5);
    toolsGrid->Add(dskmgrSizer, 1, wxEXPAND);
    
    // CPC Tool
    toolsGrid->Add(new wxStaticText(this, wxID_ANY, wxT("CPC Tool:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* cpctoolSizer = new wxBoxSizer(wxHORIZONTAL);
    m_cpcToolEdt = new wxTextCtrl(this, wxID_ANY);
    wxButton* cpctoolBtn = new wxButton(this, wxID_ANY, wxT("..."));
    cpctoolBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnCPCToolBtn, this);
    cpctoolSizer->Add(m_cpcToolEdt, 1, wxEXPAND);
    cpctoolSizer->Add(cpctoolBtn, 0, wxLEFT, 5);
    toolsGrid->Add(cpctoolSizer, 1, wxEXPAND);
    
    toolsSizer->Add(toolsGrid, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(toolsSizer, 0, wxEXPAND | wxALL, 5);
    
    // Player group
    wxStaticBox* playerBox = new wxStaticBox(this, wxID_ANY, wxT("Player"));
    wxStaticBoxSizer* playerSizer = new wxStaticBoxSizer(playerBox, wxVERTICAL);
    
    wxFlexGridSizer* playerGrid = new wxFlexGridSizer(2, 5, 5);
    playerGrid->AddGrowableCol(1);
    
    // WAV Directory
    playerGrid->Add(new wxStaticText(this, wxID_ANY, wxT("WAV Output:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* wavdirSizer = new wxBoxSizer(wxHORIZONTAL);
    m_wavDirEdt = new wxTextCtrl(this, wxID_ANY);
    wxButton* wavdirBtn = new wxButton(this, wxID_ANY, wxT("..."));
    wavdirBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnWavDirBtn, this);
    wavdirSizer->Add(m_wavDirEdt, 1, wxEXPAND);
    wavdirSizer->Add(wavdirBtn, 0, wxLEFT, 5);
    playerGrid->Add(wavdirSizer, 1, wxEXPAND);
    
    // LAME
    playerGrid->Add(new wxStaticText(this, wxID_ANY, wxT("LAME Executable:")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* lameSizer = new wxBoxSizer(wxHORIZONTAL);
    m_lameEdt = new wxTextCtrl(this, wxID_ANY);
    wxButton* lameBtn = new wxButton(this, wxID_ANY, wxT("..."));
    lameBtn->Bind(wxEVT_BUTTON, &SettingsDialog::OnLameBtn, this);
    lameSizer->Add(m_lameEdt, 1, wxEXPAND);
    lameSizer->Add(lameBtn, 0, wxLEFT, 5);
    playerGrid->Add(lameSizer, 1, wxEXPAND);
    
    // Bit Rate
    playerGrid->Add(new wxStaticText(this, wxID_ANY, wxT("MP3 Bit Rate:")), 0, wxALIGN_CENTER_VERTICAL);
    m_bitRateCmbBx = new wxComboBox(this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize,
                                     0, nullptr, wxCB_READONLY);
    m_bitRateCmbBx->Append(wxT("128"));
    m_bitRateCmbBx->Append(wxT("192"));
    m_bitRateCmbBx->Append(wxT("256"));
    m_bitRateCmbBx->Append(wxT("320"));
    playerGrid->Add(m_bitRateCmbBx, 0, wxEXPAND);
    
    playerSizer->Add(playerGrid, 0, wxEXPAND | wxALL, 5);
    
    // Checkboxes
    m_hexadecimalChkBx = new wxCheckBox(this, wxID_ANY, wxT("Display addresses and sizes in hexadecimal"));
    m_forceCubicChkBx = new wxCheckBox(this, wxID_ANY, wxT("Force cubic waveform for MP3"));
    m_infoChkBx = new wxCheckBox(this, wxID_ANY, wxT("Output info"));
    wxString tsxChoices[2];
    tsxChoices[0] = wxT("Use block #15 for payload (more compatible - OpenMSX)");
    tsxChoices[1] = wxT("Use block #19 for payload (more efficient - Maxduino)");
    m_tsxModeRadio = new wxRadioBox(this, wxID_ANY,
                                    wxT("MSX TSX encoding"),
                                    wxDefaultPosition, wxDefaultSize,
                                    2, tsxChoices,
                                    1, wxRA_SPECIFY_COLS);

    playerSizer->Add(m_hexadecimalChkBx, 0, wxALL, 5);
    playerSizer->Add(m_forceCubicChkBx, 0, wxALL, 5);
    playerSizer->Add(m_infoChkBx, 0, wxALL, 5);
    playerSizer->Add(m_tsxModeRadio, 0, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(playerSizer, 0, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxStdDialogButtonSizer* btnSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);

    // Auto-size the dialog to its content so the OK/Cancel buttons are always
    // visible (the prior fixed wxSize(500, 600) overflowed once the TSX-mode
    // radio box was added).
    SetSizerAndFit(mainSizer);
    SetMinSize(GetSize());
    Centre();
}

SettingsDialog::~SettingsDialog()
{
}

int SettingsDialog::GetBitRate() const
{
    wxString rateStr = m_bitRateCmbBx->GetStringSelection();
    long rate = 256;
    rateStr.ToLong(&rate);
    return (int)rate;
}

void SettingsDialog::SetBitRate(int rate)
{
    wxString rateStr = wxString::Format(wxT("%d"), rate);
    int index = m_bitRateCmbBx->FindString(rateStr);
    if (index != wxNOT_FOUND)
        m_bitRateCmbBx->SetSelection(index);
    else
        m_bitRateCmbBx->SetSelection(2); // Default to 256
}

void SettingsDialog::OnDirZXBtn(wxCommandEvent& event)
{
    DirDialog dlg(this);
    dlg.SetInitialDirectory(m_dirZXEdt->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        m_dirZXEdt->SetValue(dlg.GetDirectory());
}

void SettingsDialog::OnDirCPCBtn(wxCommandEvent& event)
{
    DirDialog dlg(this);
    dlg.SetInitialDirectory(m_dirCPCEdt->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        m_dirCPCEdt->SetValue(dlg.GetDirectory());
}

void SettingsDialog::OnDirMSXBtn(wxCommandEvent& event)
{
    DirDialog dlg(this);
    dlg.SetInitialDirectory(m_dirMSXEdt->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        m_dirMSXEdt->SetValue(dlg.GetDirectory());
}

void SettingsDialog::OnDir81Btn(wxCommandEvent& event)
{
    DirDialog dlg(this);
    dlg.SetInitialDirectory(m_dir81Edt->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        m_dir81Edt->SetValue(dlg.GetDirectory());
}

void SettingsDialog::OnDiskMgrBtn(wxCommandEvent& event)
{
    wxFileDialog dlg(this, wxT("Select MSX Disk Manager executable"), wxT(""), wxT(""),
                     wxT("Executable files (*.exe)|*.exe|All files (*.*)|*.*"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        m_diskMgrEdt->SetValue(dlg.GetPath());
}

void SettingsDialog::OnCPCToolBtn(wxCommandEvent& event)
{
    wxFileDialog dlg(this, wxT("Select CPC Tool executable"), wxT(""), wxT(""),
                     wxT("Executable files (*.exe)|*.exe|All files (*.*)|*.*"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        m_cpcToolEdt->SetValue(dlg.GetPath());
}

void SettingsDialog::OnWavDirBtn(wxCommandEvent& event)
{
    DirDialog dlg(this);
    dlg.SetInitialDirectory(m_wavDirEdt->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        m_wavDirEdt->SetValue(dlg.GetDirectory());
}

void SettingsDialog::OnLameBtn(wxCommandEvent& event)
{
    wxFileDialog dlg(this, wxT("Select LAME executable"), wxT(""), wxT(""),
                     wxT("Executable files (*.exe)|*.exe|All files (*.*)|*.*"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        m_lameEdt->SetValue(dlg.GetPath());
}

int SettingsDialog::GetTsxMode() const
{
    return m_tsxModeRadio->GetSelection();
}

void SettingsDialog::SetTsxMode(int v)
{
    if (v < 0 || v > 1) v = 0;
    m_tsxModeRadio->SetSelection(v);
}

void SettingsDialog::OnOK(wxCommandEvent& event)
{
    EndModal(wxID_OK);
}

void SettingsDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

