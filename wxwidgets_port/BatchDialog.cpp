#include "BatchDialog.h"
#include "DirDialog.h"
#include "MainFrame.h"
#include "../source/loaders.h"
#include "../source/zxfiles.h"
#include "../source/cpcfiles.h"
#include "../source/msxfiles.h"
#include "../source/81files.h"
#include "../source/funciones.h"
#include <wx/dirdlg.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include <cstring>

ts_sbb_header batch_head;
ts_sbb_block batch_blocks[_MAX_SBB_BLOCKS_ + 1];

int analiza_tap()
{
    int res = 0;
    int b;
    for (b = 0; b < _MAX_SBB_BLOCKS_; b++)
    {
        if (b > batch_head.n_blocks || 0 == batch_blocks[b].block_type)
            break;

        if (_zx_program_ == batch_blocks[b].block_type)
        {
            if (1 == batch_head.n_blocks)
            {
                zxbasic2bytes(batch_blocks);
                batch_head.usr_pc = batch_blocks->exec;
                break;
            }
            else
            {
                for (int i = b; i < _MAX_SBB_BLOCKS_; i++)
                {
                    batch_blocks[i] = batch_blocks[i + 1];
                }
                b--;
                batch_head.n_blocks--;
                continue;
            }
        }
        
        if (_zx_tap_hdlss_ == batch_blocks[b].block_type)
        {
            res = -11;
            break;
        }
        if (batch_blocks[b].ini + batch_blocks[b].size - 1 >= MAX_DIR_ZX)
        {
            for (int i = _MAX_SBB_BLOCKS_; i > b; i--)
            {
                batch_blocks[i] = batch_blocks[i - 1];
            }
            zxlastbytes(&batch_head, &batch_blocks[b]);
            batch_head.n_blocks++;
            b++;
        }
    }
    return res;
}


BatchDialog::BatchDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Batch Conversion"), wxDefaultPosition, wxSize(500, 400))
{
    m_maquina = 'Z';
    m_tipo = 1;
    m_m_y_e = 0;
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Input directory
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    inputSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Input dir:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_inputDirEdt = new wxTextCtrl(this, wxID_ANY);
    inputSizer->Add(m_inputDirEdt, 1, wxEXPAND | wxRIGHT, 5);
    wxButton* inputDirBtn = new wxButton(this, wxID_ANY, wxT("..."));
    inputSizer->Add(inputDirBtn, 0);
    mainSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 5);
    
    // Output directory
    wxBoxSizer* outputSizer = new wxBoxSizer(wxHORIZONTAL);
    outputSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Output dir:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_outputDirEdt = new wxTextCtrl(this, wxID_ANY);
    outputSizer->Add(m_outputDirEdt, 1, wxEXPAND | wxRIGHT, 5);
    wxButton* outputDirBtn = new wxButton(this, wxID_ANY, wxT("..."));
    outputSizer->Add(outputDirBtn, 0);
    mainSizer->Add(outputSizer, 0, wxEXPAND | wxALL, 5);
    
    // Extension combo
    m_label2 = new wxStaticText(this, wxID_ANY, wxT("Select file type:"));
    mainSizer->Add(m_label2, 0, wxALL, 5);
    m_extensionCmbBx = new wxComboBox(this, wxID_ANY);
    mainSizer->Add(m_extensionCmbBx, 0, wxEXPAND | wxALL, 5);
    
    // Make MP3 checkbox
    m_makeMp3ChkBx = new wxCheckBox(this, wxID_ANY, wxT("Make MP3"));
    mainSizer->Add(m_makeMp3ChkBx, 0, wxALL, 5);
    
    // Progress bar
    m_progressBar = new wxGauge(this, wxID_ANY, 100);
    mainSizer->Add(m_progressBar, 0, wxEXPAND | wxALL, 5);
    
    // Memo
    m_memo = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(-1, 150),
                            wxTE_MULTILINE | wxTE_READONLY);
    mainSizer->Add(m_memo, 1, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxButton* proceedBtn = new wxButton(this, wxID_ANY, wxT("Proceed"));
    wxButton* cancelBtn = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->Add(proceedBtn, 0, wxRIGHT, 5);
    btnSizer->Add(cancelBtn, 0);
    mainSizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 5);
    
    inputDirBtn->Bind(wxEVT_BUTTON, &BatchDialog::OnInputDir, this);
    outputDirBtn->Bind(wxEVT_BUTTON, &BatchDialog::OnOutputDir, this);
    proceedBtn->Bind(wxEVT_BUTTON, &BatchDialog::OnProceed, this);
    
    SetSizer(mainSizer);
    Centre();
}

void BatchDialog::Batch(char m, int t)
{
    m_maquina = m;
    m_tipo = t;
    m_memo->Clear();
    m_extensionCmbBx->Clear();
    
    switch (m)
    {
        case 'Z':
            m_extensionCmbBx->Append(wxT(".Z80"));
            m_extensionCmbBx->Append(wxT(".SNA"));
            m_extensionCmbBx->Append(wxT(".TAP"));
            break;
        case 'C':
            m_extensionCmbBx->Append(wxT(".SNA"));
            m_extensionCmbBx->Append(wxT(".BIN"));
            break;
        case 'M':
            m_extensionCmbBx->Append(wxT(".ROM"));
            m_extensionCmbBx->Append(wxT(".CAS"));
            m_extensionCmbBx->Append(wxT(".BIN"));
            break;
        case '8':
            m_extensionCmbBx->Append(wxT(".P"));
            m_extensionCmbBx->Append(wxT(".Z81"));
            break;
    }
    
    wxString labelText = wxT("Select file type for ") + wxString(NombreMachine(m), wxConvLocal);
    m_label2->SetLabel(labelText);
    m_extensionCmbBx->SetSelection(0);
    
    if (t == 1)
    {
        SetTitle(wxT("Converting to .SBB ..."));
        m_label2->Show();
        m_extensionCmbBx->Show();
        m_makeMp3ChkBx->Hide();
    }
    else
    {
        SetTitle(wxT("Converting to .WAV ..."));
        m_label2->Hide();
        m_extensionCmbBx->Hide();
        m_makeMp3ChkBx->Show();
    }
    
    ShowModal();
}

void BatchDialog::OnInputDir(wxCommandEvent& event)
{
    DirDialog dlg(this);
    dlg.SetInitialDirectory(m_inputDirEdt->GetValue());
    if (dlg.ShowModal() == wxID_OK)
    {
        m_inputDirEdt->SetValue(dlg.GetDirectory());
    }
}

void BatchDialog::OnOutputDir(wxCommandEvent& event)
{
    DirDialog dlg(this);
    dlg.SetInitialDirectory(m_outputDirEdt->GetValue());
    if (dlg.ShowModal() == wxID_OK)
    {
        m_outputDirEdt->SetValue(dlg.GetDirectory());
    }
}

void BatchDialog::OnProceed(wxCommandEvent& event)
{
    if (m_tipo == 1)
    {
        wxString ext = m_extensionCmbBx->GetValue();
        if (ext.Length() >= 2)
            m_m_y_e = (m_maquina << 8) + static_cast<unsigned char>(ext[1]);
    }
    
    if (m_tipo == 1)
        ConvertToSbb();
    else if (m_tipo == 2)
        ConvertToWav();
}

int BatchDialog::BuildSbb(const wxString& file)
{
    int res;
    wxString mbPath = file.mb_str(wxConvLocal);
    
    switch (m_m_y_e)
    {
        case 0x5a54: // ZT
            res = leetap(mbPath.c_str(), &batch_head, batch_blocks);
            if (!res) res = analiza_tap();
            break;
        case 0x5a53: // ZS
            res = lee_zx_sna(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        case 0x5a5a: // ZZ
            res = leez80(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        case 0x4342: // CB
            res = lee_cpc_bin(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        case 0x4353: // CS
            res = lee_cpc_sna(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        case 0x4d42: // MB
            res = lee_msx_bin(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        case 0x4d43: // MC
            res = leecas(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        case 0x4d52: // MR
            res = lee_msx_rom(mbPath.c_str(), &batch_head, batch_blocks, nullptr);
            break;
        case 0x3850: // 8P
            res = lee_p(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        case 0x385a: // 8Z
            res = lee_z81(mbPath.c_str(), &batch_head, batch_blocks);
            break;
        default:
            res = -1;
    }
    return res;
}

int BatchDialog::N_Files(const wxString& mask)
{
    wxArrayString files;
    wxDir::GetAllFiles(wxFileName(mask).GetPath(), &files, wxFileName(mask).GetFullName());
    return files.GetCount();
}

void BatchDialog::ConvertToSbb()
{
    wxString mask = m_inputDirEdt->GetValue();
    if (!mask.IsEmpty() && mask.Last() != wxFILE_SEP_PATH)
        mask += wxFILE_SEP_PATH;
    mask += wxT("*");
    mask += m_extensionCmbBx->GetValue();
    
    wxArrayString files;
    wxDir::GetAllFiles(wxFileName(mask).GetPath(), &files, wxFileName(mask).GetFullName());
    
    m_progressBar->SetRange(files.GetCount());
    m_progressBar->SetValue(0);
    
    for (size_t i = 0; i < files.GetCount(); i++)
    {
        m_progressBar->SetValue(i + 1);
        
        wxString fileIn = files[i];
        wxString fileOut = m_outputDirEdt->GetValue();
        if (!fileOut.IsEmpty() && fileOut.Last() != wxFILE_SEP_PATH)
            fileOut += wxFILE_SEP_PATH;
        fileOut += wxFileName(fileIn).GetName() + wxT(".sbb");
        
        wxString linea = wxT("Converting ") + wxFileName(fileIn).GetFullName();
        
        int res = BuildSbb(fileIn);
        if (!res)
            res = save_sbb_file(fileOut.mb_str(wxConvLocal), &batch_head, batch_blocks);
        
        if (res)
            linea += wxT("  error");
        else
            linea += wxT("  OK");
        
        m_memo->AppendText(linea + wxT("\n"));
        wxYield(); // Allow UI updates
    }
}

void BatchDialog::ConvertToWav()
{
    wxString mask = m_inputDirEdt->GetValue();
    if (!mask.IsEmpty() && mask.Last() != wxFILE_SEP_PATH)
        mask += wxFILE_SEP_PATH;
    mask += wxT("*.sbb");
    
    wxArrayString files;
    wxDir::GetAllFiles(wxFileName(mask).GetPath(), &files, wxT("*.sbb"));
    
    m_progressBar->SetRange(files.GetCount());
    m_progressBar->SetValue(0);
    
    MainFrame* mainFrame = dynamic_cast<MainFrame*>(GetParent());
    
    for (size_t i = 0; i < files.GetCount(); i++)
    {
        m_progressBar->SetValue(i + 1);
        
        wxString fileIn = files[i];
        wxString fileOut = m_outputDirEdt->GetValue();
        if (!fileOut.IsEmpty() && fileOut.Last() != wxFILE_SEP_PATH)
            fileOut += wxFILE_SEP_PATH;
        fileOut += wxFileName(fileIn).GetName() + wxT(".wav");
        
        wxString linea = wxT("converting ") + wxFileName(fileIn).GetFullName();
        
        int res = sbb2wav(fileIn.mb_str(wxConvLocal), fileOut.mb_str(wxConvLocal));
        
        if (res)
        {
            linea += wxT("  error");
        }
        else
        {
            linea += wxT("  OK");
            if (m_makeMp3ChkBx->IsChecked() && mainFrame)
            {
                extern char nombre_head[17];
                mainFrame->LameProcess(fileOut, m_maquina, nombre_head);
            }
        }
        
        m_memo->AppendText(linea + wxT("\n"));
        wxYield(); // Allow UI updates
    }
}

