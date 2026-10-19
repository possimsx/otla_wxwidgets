#include "AdjustDialog.h"
#include "WavMonitorDialog.h"
#include "../source/signaltest.h"
#include "../source/wav2.h"
#include "../source/sbb.h"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <cstring>
#include <string>

extern int invert_pol;
extern int forma_onda;
extern int freq_muestreo;
extern char s_p_b;


AdjustDialog::AdjustDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Adjust Audio"), wxDefaultPosition, wxSize(400, 300))
{
    m_maquina = 'Z';
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Program group
    wxStaticBoxSizer* programSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Program"));
    wxButton* makeProgramBtn = new wxButton(this, wxID_ANY, wxT("Make Program"));
    wxButton* playProgramBtn = new wxButton(this, wxID_ANY, wxT("Play Program"));
    programSizer->Add(makeProgramBtn, 0, wxEXPAND | wxALL, 5);
    programSizer->Add(playProgramBtn, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(programSizer, 0, wxEXPAND | wxALL, 5);
    
    // Signal group
    wxStaticBoxSizer* signalSizer = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Test Signal"));
    
    wxBoxSizer* freqSizer = new wxBoxSizer(wxHORIZONTAL);
    freqSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Frequency:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_frecuenciaCmbBx = new wxComboBox(this, wxID_ANY);
    m_frecuenciaCmbBx->Append(wxT("44.1 kHz"));
    m_frecuenciaCmbBx->Append(wxT("48 kHz"));
    m_frecuenciaCmbBx->SetSelection(0);
    freqSizer->Add(m_frecuenciaCmbBx, 1, wxEXPAND);
    signalSizer->Add(freqSizer, 0, wxEXPAND | wxALL, 5);
    
    wxBoxSizer* formaSizer = new wxBoxSizer(wxHORIZONTAL);
    formaSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Waveform:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_formaCmbBx = new wxComboBox(this, wxID_ANY);
    m_formaCmbBx->Append(wxT("Square"));
    m_formaCmbBx->Append(wxT("Sine"));
    m_formaCmbBx->Append(wxT("Triangle"));
    m_formaCmbBx->Append(wxT("Sawtooth"));
    m_formaCmbBx->SetSelection(0);
    formaSizer->Add(m_formaCmbBx, 1, wxEXPAND);
    signalSizer->Add(formaSizer, 0, wxEXPAND | wxALL, 5);
    
    m_invertChkBx = new wxCheckBox(this, wxID_ANY, wxT("Invert"));
    signalSizer->Add(m_invertChkBx, 0, wxALL, 5);
    
    wxBoxSizer* testSizer = new wxBoxSizer(wxHORIZONTAL);
    testSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Test Signal:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_testSignalCmbBx = new wxComboBox(this, wxID_ANY);
    testSizer->Add(m_testSignalCmbBx, 1, wxEXPAND);
    signalSizer->Add(testSizer, 0, wxEXPAND | wxALL, 5);
    
    wxButton* makeSignalBtn = new wxButton(this, wxID_ANY, wxT("Make Signal"));
    wxButton* playSignalBtn = new wxButton(this, wxID_ANY, wxT("Play Signal"));
    signalSizer->Add(makeSignalBtn, 0, wxEXPAND | wxALL, 5);
    signalSizer->Add(playSignalBtn, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(signalSizer, 0, wxEXPAND | wxALL, 5);
    
    // Close button
    wxButton* closeBtn = new wxButton(this, wxID_CLOSE, wxT("Close"));
    mainSizer->Add(closeBtn, 0, wxALIGN_RIGHT | wxALL, 5);
    
    makeProgramBtn->Bind(wxEVT_BUTTON, &AdjustDialog::OnMakeProgram, this);
    playProgramBtn->Bind(wxEVT_BUTTON, &AdjustDialog::OnPlayProgram, this);
    makeSignalBtn->Bind(wxEVT_BUTTON, &AdjustDialog::OnMakeSignal, this);
    playSignalBtn->Bind(wxEVT_BUTTON, &AdjustDialog::OnPlaySignal, this);
    closeBtn->Bind(wxEVT_BUTTON, &AdjustDialog::OnClose, this);
    m_frecuenciaCmbBx->Bind(wxEVT_COMBOBOX, &AdjustDialog::OnFrequencyChange, this);
    
    SetSizer(mainSizer);
    Centre();
    
    OnFrequencyChange(wxCommandEvent());
}

void AdjustDialog::SetMachine(char maquina)
{
    m_maquina = maquina;
    SetTitle(wxString::Format(wxT("Adjust test for %s"),
                              wxString(NombreMachine(maquina), wxConvLocal).c_str()));
}

void AdjustDialog::SetOutputDir(const wxString& dir)
{
    m_outputDir = dir;
}

void AdjustDialog::SetFrequency(int idx)
{
    if (idx >= 0 && idx < (int)m_frecuenciaCmbBx->GetCount())
    {
        m_frecuenciaCmbBx->SetSelection(idx);
        wxCommandEvent dummy;
        OnFrequencyChange(dummy);
    }
}

void AdjustDialog::OnMakeProgram(wxCommandEvent& event)
{
    int res = -1;
    wxString programWav;
    
    switch (m_maquina)
    {
        case 'Z':
            programWav = m_outputDir + wxFILE_SEP_PATH + wxT("adjust_zx.wav");
            {
                std::string path = programWav.mb_str(wxConvLocal).data();
                res = adjust_zx(&path[0]);
            }
            break;
        case 'C':
            programWav = m_outputDir + wxFILE_SEP_PATH + wxT("adjust_cpc.wav");
            {
                std::string path = programWav.mb_str(wxConvLocal).data();
                res = adjust_cpc(&path[0]);
            }
            break;
        case 'M':
            programWav = m_outputDir + wxFILE_SEP_PATH + wxT("adjust_msx.wav");
            {
                std::string path = programWav.mb_str(wxConvLocal).data();
                res = adjust_msx(&path[0]);
            }
            break;
        case '8':
            programWav = m_outputDir + wxFILE_SEP_PATH + wxT("adjust_81.wav");
            {
                std::string path = programWav.mb_str(wxConvLocal).data();
                res = adjust_81(&path[0]);
            }
            break;
    }
    
    if (!res)
    {
        m_programWav = programWav;
        wxMessageBox(programWav + wxT("\r\nhas been made"), wxT("Success"), wxOK | wxICON_INFORMATION);
    }
    else
    {
        wxMessageBox(wxT("ERROR!!! with\r\n") + programWav, wxT("Error"), wxOK | wxICON_ERROR);
    }
}

void AdjustDialog::OnMakeSignal(wxCommandEvent& event)
{
    char tone[6 * 0x400];
    char f_c;
    int div = 2;
    
    switch (m_testSignalCmbBx->GetSelection())
    {
        case 1: f_c = 0x55; break;
        case 2: f_c = 0xaa; break;
        case 3: f_c = 0xff; break;
        default: f_c = 0x00; break;
    }
    memset(tone, f_c, sizeof(tone));
    
    s_p_b = _2_50;
    
    if (m_invertChkBx->IsChecked())
        invert_pol = -1;
    else
        invert_pol = 1;
    
    forma_onda = m_formaCmbBx->GetSelection();
    
    wxString signalName = m_outputDir + wxFILE_SEP_PATH + wxT("signal_");
    
    if (m_frecuenciaCmbBx->GetSelection())
    {
        freq_muestreo = 48000;
        switch (m_testSignalCmbBx->GetSelection())
        {
            case 1: signalName += wxT("12"); break;
            case 0: signalName += wxT("24"); div = 1; break;
            default: break;
        }
    }
    else
    {
        freq_muestreo = 44100;
        switch (m_testSignalCmbBx->GetSelection())
        {
            case 1: signalName += wxT("11"); break;
            case 0: signalName += wxT("22"); div = 1; break;
            default: break;
        }
    }
    signalName += wxT("kHz.wav");
    
    int r = open_wav(signalName.mb_str(wxConvLocal).data());
    if (r != 0)
    {
        wxMessageBox(wxT("ERROR creating\r\n") + signalName, wxT("Error"), wxOK | wxICON_ERROR);
        return;
    }

    for (int i = 0; i < 30; i++)
    {
        pseudopause(900 - i);
        pilot_veloz(25);
        block2veloz_wav(reinterpret_cast<const unsigned char*>(tone), sizeof(tone) / div - 1);
    }
    close_wav();

    m_signalName = signalName;
    wxMessageBox(signalName + wxT("\r\nhas been made"), wxT("Success"), wxOK | wxICON_INFORMATION);
}

void AdjustDialog::OnClose(wxCommandEvent& event)
{
    EndModal(wxID_CLOSE);
}

void AdjustDialog::OnFrequencyChange(wxCommandEvent& event)
{
    int s = m_testSignalCmbBx->GetSelection();
    
    m_testSignalCmbBx->Clear();
    if (m_frecuenciaCmbBx->GetSelection())
    {
        m_testSignalCmbBx->Append(wxT("24 kHz"));
        m_testSignalCmbBx->Append(wxT("12 kHz"));
    }
    else
    {
        m_testSignalCmbBx->Append(wxT("22 kHz"));
        m_testSignalCmbBx->Append(wxT("11 kHz"));
    }
    if (s >= 0 && s < (int)m_testSignalCmbBx->GetCount())
        m_testSignalCmbBx->SetSelection(s);
    else
        m_testSignalCmbBx->SetSelection(1);
}

void AdjustDialog::OnPlayProgram(wxCommandEvent& event)
{
    OnMakeProgram(event);
    if (!m_programWav.IsEmpty() && wxFileExists(m_programWav))
    {
        WavMonitorDialog dlg(this);
        if (dlg.OpenWaveFile(m_programWav) == 0)
        {
            dlg.ShowModal();
        }
    }
}

void AdjustDialog::OnPlaySignal(wxCommandEvent& event)
{
    OnMakeSignal(event);
    if (!m_signalName.IsEmpty() && wxFileExists(m_signalName))
    {
        WavMonitorDialog dlg(this);
        if (dlg.OpenWaveFile(m_signalName) == 0)
        {
            dlg.ShowModal();
        }
    }
}

