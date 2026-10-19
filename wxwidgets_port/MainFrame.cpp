#include "MainFrame.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "DirDialog.h"
#include "ModifyDialog.h"
#include "Dialogs.h"

#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/utils.h>
#include <wx/image.h>
#include <wx/dir.h>
#include <wx/settings.h>
#include <cstring>
#include <string.h>

// Include embedded BMP resources if available
#ifdef __has_include
    #if __has_include("embedded_bmp_resources.h")
        #include "embedded_bmp_resources.h"
        #define HAVE_EMBEDDED_BMP 1
    #endif
#endif
#ifndef HAVE_EMBEDDED_BMP
    // Fallback: try to include if it exists (for older compilers)
    #ifdef _MSC_VER
        #pragma message("Note: embedded_bmp_resources.h not found, using file-based loading")
    #endif
    #define HAVE_EMBEDDED_BMP 0
#endif

// Include SBB functions
#include "../source/sbb.h"
#include "../source/funciones.h"
#include "../source/wav2.h"
#include "../source/loaders.h"

// Direct SBB -> TSX generator (no WAV intermediate)
#include "tsx/sbb2tsx.h"

// Tool paths - accessible from dialogs
wxString ToolDiskMgr;
wxString ToolCPC;

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_NEW, MainFrame::OnNew)
    EVT_MENU(wxID_OPEN, MainFrame::OnOpen)
    EVT_MENU(wxID_SAVE, MainFrame::OnSave)
    EVT_MENU(wxID_SAVEAS, MainFrame::OnSaveAs)
    EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_MENU(wxID_PREFERENCES, MainFrame::OnSettings)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_COMBOBOX(wxID_ANY, MainFrame::OnMachineChange)
    EVT_BUTTON(wxID_ANY, MainFrame::OnAddButton)
    EVT_TEXT(wxID_ANY, MainFrame::OnNameChange)
wxEND_EVENT_TABLE()

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, wxT("O.T.L.A. 2.2.5"), wxDefaultPosition, wxSize(400, 800))
{
    // Initialize data
    m_nuevo = false;
    m_modificado = false;
    m_showInfo = true;
    m_viewHex = false;
    m_bitRateMp3 = 256;
    m_forceCubic = true;
    m_outputInfo = false;
    m_tsxMode = 0; // #4B + #15
    
    // Create menu bar
    m_menuBar = new wxMenuBar();
    
    // File menu
    m_fileMenu = new wxMenu();
    m_fileMenu->Append(wxID_NEW, wxT("&New\tCtrl+N"));
    m_fileMenu->Append(wxID_OPEN, wxT("&Open...\tCtrl+O"));
    m_fileMenu->Append(wxID_SAVE, wxT("&Save\tCtrl+S"));
    m_fileMenu->Append(wxID_SAVEAS, wxT("Save &As..."));
    m_fileMenu->AppendSeparator();
    m_fileMenu->Append(wxID_EXIT, wxT("&Quit\tCtrl+Q"));
    m_menuBar->Append(m_fileMenu, wxT("&File"));
    
    // Convert menu
    m_convertMenu = new wxMenu();
    m_wavMenuItem = m_convertMenu->Append(wxID_ANY, wxT("&WAV..."));
    m_mp3MenuItem = m_convertMenu->Append(wxID_ANY, wxT("&MP3..."));
    m_tzxMenuItem = m_convertMenu->Append(wxID_ANY, wxT("&TZX..."));
    m_menuBar->Append(m_convertMenu, wxT("&Convert"));
    
    // Tools menu
    m_toolsMenu = new wxMenu();
    m_toolsMenu->Append(wxID_ANY, wxT("&Batch SBB..."));
    m_toolsMenu->Append(wxID_ANY, wxT("Batch &WAV..."));
    m_toolsMenu->Append(wxID_ANY, wxT("&Adjust Audio..."));
    m_toolsMenu->AppendSeparator();
    m_toolsMenu->Append(wxID_ANY, wxT("&Edit Blocks..."));
    m_toolsMenu->Append(wxID_ANY, wxT("&WAV Player..."));
    m_toolsMenu->AppendSeparator();
    m_toolsMenu->Append(wxID_PREFERENCES, wxT("&Settings..."));
    m_menuBar->Append(m_toolsMenu, wxT("&Tools"));
    
    // Help menu
    m_helpMenu = new wxMenu();
    m_helpMenu->Append(wxID_ABOUT, wxT("&About..."));
    m_helpMenu->Append(wxID_ANY, wxT("&Wiki..."));
    m_menuBar->Append(m_helpMenu, wxT("&Help"));
    
    SetMenuBar(m_menuBar);

#ifdef _WIN32
    SetIcon(wxICON(IDI_ICON1));
#endif

    // Create main panel
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Machine selection at top
    wxBoxSizer* machineSizer = new wxBoxSizer(wxHORIZONTAL);
    // Small left spacer so "Machine:" aligns with the "Builder" group-box title
    machineSizer->AddSpacer(8);
    machineSizer->Add(new wxStaticText(panel, wxID_ANY, wxT("Machine:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    // Initialize image list for machines with WIDE format (140x46)
    m_machineImgLst = new wxImageList(140, 46);
    LoadMachineIcons();

    // Combo width just enough for the 140 px machine logos + dropdown arrow
    m_machineCmbBx = new wxBitmapComboBox(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(170, -1),
                                           0, nullptr, wxCB_READONLY);
    
    // Add items with wide logos
    wxBitmap bmpZX, bmpCPC, bmpMSX, bmp81;
    if (m_machineImgLst->GetImageCount() >= 4)
    {
        bmpZX = m_machineImgLst->GetBitmap(0);
        bmpCPC = m_machineImgLst->GetBitmap(1);
        bmpMSX = m_machineImgLst->GetBitmap(2);
        bmp81 = m_machineImgLst->GetBitmap(3);
    }
    
    // Add items with ONLY images (no text labels - images are self-explanatory)
    m_machineCmbBx->Append(wxT(""), bmpZX.IsOk() ? bmpZX : wxNullBitmap);
    m_machineCmbBx->Append(wxT(""), bmpCPC.IsOk() ? bmpCPC : wxNullBitmap);
    m_machineCmbBx->Append(wxT(""), bmpMSX.IsOk() ? bmpMSX : wxNullBitmap);
    m_machineCmbBx->Append(wxT(""), bmp81.IsOk() ? bmp81 : wxNullBitmap);
    
    machineSizer->Add(m_machineCmbBx, 0);
    machineSizer->AddStretchSpacer(1);
    mainSizer->Add(machineSizer, 0, wxEXPAND | wxALL, 5);
    
    // Builder group
    m_builderGB = new wxStaticBox(panel, wxID_ANY, wxT("Builder"));
    wxStaticBoxSizer* builderSizer = new wxStaticBoxSizer(m_builderGB, wxVERTICAL);
    
    // File name at top of builder
    m_fileEdt = new wxTextCtrl(panel, wxID_ANY, wxT("new.SBB"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
    wxFont boldFont = m_fileEdt->GetFont();
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);
    m_fileEdt->SetFont(boldFont);
    builderSizer->Add(m_fileEdt, 0, wxEXPAND | wxALL, 5);
    
    // Header group - reorganized for clarity
    m_headerGB = new wxStaticBox(panel, wxID_ANY, wxT("Header"));
    wxStaticBoxSizer* headerSizer = new wxStaticBoxSizer(m_headerGB, wxVERTICAL);
    
    // Row 1: Model and Enable interrupts (top right corner)
    wxBoxSizer* topRowSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_modelLbl = new wxStaticText(panel, wxID_ANY, wxT("Model:"));
    m_modelCmbBx = new wxComboBox(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(120, -1),
                                   0, nullptr, wxCB_READONLY);
    topRowSizer->Add(m_modelLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    topRowSizer->Add(m_modelCmbBx, 0, wxRIGHT, 10);
    
    m_enableIntChkBx = new wxCheckBox(panel, wxID_ANY, wxT("Enable interrupts"));
    topRowSizer->Add(m_enableIntChkBx, 0, wxALIGN_CENTER_VERTICAL);
    
    headerSizer->Add(topRowSizer, 0, wxALL, 5);
    
    // Row 2: Locate and POKE (machine-specific fields)
    wxFlexGridSizer* machineSpecificGrid = new wxFlexGridSizer(4, 5, 5);
    m_locateLbl = new wxStaticText(panel, wxID_ANY, wxT("Locate at:"));
    m_reubicaEdt = new wxTextCtrl(panel, wxID_ANY, wxT("244"), wxDefaultPosition, wxSize(60, -1));
    m_pokeLbl = new wxStaticText(panel, wxID_ANY, wxT("POKE -1:"));
    m_pokeEdt = new wxTextCtrl(panel, wxID_ANY, wxT("0"), wxDefaultPosition, wxSize(60, -1));
    machineSpecificGrid->Add(m_locateLbl, 0, wxALIGN_CENTER_VERTICAL);
    machineSpecificGrid->Add(m_reubicaEdt, 0);
    machineSpecificGrid->Add(m_pokeLbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    machineSpecificGrid->Add(m_pokeEdt, 0);
    headerSizer->Add(machineSpecificGrid, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Row 3: Loading name (full width)
    wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    m_nameLbl = new wxStaticText(panel, wxID_ANY, wxT("Loading name:"));
    m_nameEdt = new wxTextCtrl(panel, wxID_ANY, wxT(""));
    nameSizer->Add(m_nameLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    nameSizer->Add(m_nameEdt, 1, wxEXPAND);
    headerSizer->Add(nameSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Row 4: Additional info (full width)
    wxBoxSizer* additionalSizer = new wxBoxSizer(wxHORIZONTAL);
    m_additionalLbl = new wxStaticText(panel, wxID_ANY, wxT("Additional info:"));
    m_additionalInfoEdt = new wxTextCtrl(panel, wxID_ANY, wxT(""));
    additionalSizer->Add(m_additionalLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    additionalSizer->Add(m_additionalInfoEdt, 1, wxEXPAND);
    headerSizer->Add(additionalSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Row 5: Add button, Blocks counter, Origin
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);
    m_addBtn = new wxButton(panel, wxID_ANY, wxT("Add blocks..."), wxDefaultPosition, wxDefaultSize);
    controlsSizer->Add(m_addBtn, 0, wxRIGHT, 10);
    
    m_blocksLbl = new wxStaticText(panel, wxID_ANY, wxT("Blocks:"));
    m_blocksEdt = new wxTextCtrl(panel, wxID_ANY, wxT("0"), wxDefaultPosition, wxSize(35, -1), wxTE_READONLY);
    controlsSizer->Add(m_blocksLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
    controlsSizer->Add(m_blocksEdt, 0, wxRIGHT, 10);
    
    wxStaticText* originLbl = new wxStaticText(panel, wxID_ANY, wxT("Origin:"));
    m_originEdt = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(35, -1), wxTE_READONLY);
    controlsSizer->Add(originLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 3);
    controlsSizer->Add(m_originEdt, 0);
    
    // Row 5: CLEAR and USR (execution parameters)
    wxFlexGridSizer* execGrid = new wxFlexGridSizer(4, 5, 5);
    m_clearLbl = new wxStaticText(panel, wxID_ANY, wxT("CLEAR:"));
    m_clearEdt = new wxTextCtrl(panel, wxID_ANY, wxT("0"), wxDefaultPosition, wxSize(80, -1));
    m_usrLbl = new wxStaticText(panel, wxID_ANY, wxT("JP:"));
    m_usrEdt = new wxTextCtrl(panel, wxID_ANY, wxT("0"), wxDefaultPosition, wxSize(80, -1));
    execGrid->Add(m_clearLbl, 0, wxALIGN_CENTER_VERTICAL);
    execGrid->Add(m_clearEdt, 0);
    execGrid->Add(m_usrLbl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    execGrid->Add(m_usrEdt, 0);
    headerSizer->Add(execGrid, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

    builderSizer->Add(headerSizer, 0, wxEXPAND | wxALL, 5);

    // Add blocks / Blocks / Origin — placed directly above the block list
    builderSizer->Add(controlsSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);

    // Blocks list
    m_blocksLV = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxSize(-1, 110),
                                wxLC_REPORT | wxLC_SINGLE_SEL);
    m_blocksLV->InsertColumn(0, wxT("Block name"), wxLIST_FORMAT_LEFT, 100);
    m_blocksLV->InsertColumn(1, wxT("Size"), wxLIST_FORMAT_RIGHT, 50);
    m_blocksLV->InsertColumn(2, wxT("Type"), wxLIST_FORMAT_LEFT, 80);
    m_blocksLV->InsertColumn(3, wxT("Start"), wxLIST_FORMAT_LEFT, 60);
    m_blocksLV->InsertColumn(4, wxT("Jump"), wxLIST_FORMAT_LEFT, 0);  // Hidden column
    m_blocksLV->InsertColumn(5, wxT("Exec"), wxLIST_FORMAT_LEFT, 60);
    builderSizer->Add(m_blocksLV, 1, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(builderSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);
    
    // Player group
    m_playerGB = new wxStaticBox(panel, wxID_ANY, wxT("Player"));
    wxStaticBoxSizer* playerSizer = new wxStaticBoxSizer(m_playerGB, wxVERTICAL);
    
    // Horizontal layout: Loader on left, WAV on right
    wxBoxSizer* settingsHSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Loader group (left side)
    m_loaderGB = new wxStaticBox(panel, wxID_ANY, wxT("Loader Settings"));
    wxStaticBoxSizer* loaderSizer = new wxStaticBoxSizer(m_loaderGB, wxVERTICAL);
    
    m_makeLoaderChkBx = new wxCheckBox(panel, wxID_ANY, wxT("Make loader"));
    m_accelerateChkBx = new wxCheckBox(panel, wxID_ANY, wxT("Accelerate"));
    m_checkLoadErrorChkBx = new wxCheckBox(panel, wxID_ANY, wxT("Check load error"));
    m_releMotorChkBx = new wxCheckBox(panel, wxID_ANY, wxT("Motor relay"));
    
    loaderSizer->Add(m_makeLoaderChkBx, 0, wxALL, 3);
    loaderSizer->Add(m_accelerateChkBx, 0, wxALL, 3);
    loaderSizer->Add(m_checkLoadErrorChkBx, 0, wxALL, 3);
    loaderSizer->Add(m_releMotorChkBx, 0, wxALL, 3);
    
    settingsHSizer->Add(loaderSizer, 0, wxEXPAND | wxALL, 5);
    
    // WAV group (right side)
    m_wavGB = new wxStaticBox(panel, wxID_ANY, wxT("Wav Settings"));
    wxStaticBoxSizer* wavSizer = new wxStaticBoxSizer(m_wavGB, wxVERTICAL);
    
    wxFlexGridSizer* wavGrid = new wxFlexGridSizer(2, 3, 3);
    wavGrid->AddGrowableCol(1);
    
    wavGrid->Add(new wxStaticText(panel, wxID_ANY, wxT("Frequency:")), 0, wxALIGN_CENTER_VERTICAL);
    m_frecuenciaCmbBx = new wxComboBox(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize,
                                       0, nullptr, wxCB_READONLY);
    m_frecuenciaCmbBx->Append(wxT("44100 Hz"));
    m_frecuenciaCmbBx->Append(wxT("48000 Hz"));
    wavGrid->Add(m_frecuenciaCmbBx, 0, wxEXPAND);
    
    wavGrid->Add(new wxStaticText(panel, wxID_ANY, wxT("Samples per bit:")), 0, wxALIGN_CENTER_VERTICAL);
    m_muestrasCmbBx = new wxComboBox(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize,
                                     0, nullptr, wxCB_READONLY);
    m_muestrasCmbBx->Append(wxT("1.75"));
    m_muestrasCmbBx->Append(wxT("2.25"));
    m_muestrasCmbBx->Append(wxT("2.50"));
    m_muestrasCmbBx->Append(wxT("2.75"));
    m_muestrasCmbBx->Append(wxT("3.00"));
    m_muestrasCmbBx->Append(wxT("3.50"));
    m_muestrasCmbBx->Append(wxT("3.75"));
    m_muestrasCmbBx->Append(wxT("4.00"));
    m_muestrasCmbBx->Append(wxT("4.25"));
    wavGrid->Add(m_muestrasCmbBx, 0, wxEXPAND);
    
    wavGrid->Add(new wxStaticText(panel, wxID_ANY, wxT("Waveform:")), 0, wxALIGN_CENTER_VERTICAL);
    m_formaCmbBx = new wxBitmapComboBox(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize,
                                         0, nullptr, wxCB_READONLY);
    // Create waveform icons and populate combo box
    m_waveformImgLst = new wxImageList(24, 24);
    CreateWaveformIcons();
    wavGrid->Add(m_formaCmbBx, 0, wxEXPAND);
    
    wavGrid->Add(new wxStaticText(panel, wxID_ANY, wxT("Pause (ms):")), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* pauseSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pausaEdt = new wxTextCtrl(panel, wxID_ANY, wxT("500"), wxDefaultPosition, wxSize(50, -1));
    pauseSizer->Add(m_pausaEdt, 0);
    wavGrid->Add(pauseSizer, 0, wxEXPAND);
    
    wavSizer->Add(wavGrid, 0, wxEXPAND | wxALL, 5);
    
    m_invertChkBx = new wxCheckBox(panel, wxID_ANY, wxT("Invert polarity"));
    wavSizer->Add(m_invertChkBx, 0, wxALL, 5);
    
    settingsHSizer->Add(wavSizer, 1, wxEXPAND | wxALL, 5);
    
    playerSizer->Add(settingsHSizer, 0, wxEXPAND);
    
    // Buttons (initial labels; the TZX button label is updated per machine
    // by ChangeMachine — e.g. "SBB => CDT" on CPC, "SBB => TZX?" on MSX)
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    m_playBtn = new wxButton(panel, wxID_ANY, wxT("Play"));
    m_wavBtn = new wxButton(panel, wxID_ANY, wxT("SBB => WAV"));
    m_mp3Btn = new wxButton(panel, wxID_ANY, wxT("SBB => MP3"));
    m_tzxBtn = new wxButton(panel, wxID_ANY, wxT("SBB => TZX"));
    
    buttonSizer->AddSpacer(10);
    buttonSizer->Add(m_playBtn, 0, wxALL, 2);
    buttonSizer->Add(m_wavBtn, 0, wxALL, 2);
    buttonSizer->Add(m_mp3Btn, 0, wxALL, 2);
    buttonSizer->Add(m_tzxBtn, 0, wxALL, 2);
    
    playerSizer->Add(buttonSizer, 0, wxALIGN_LEFT | wxALL, 5);
    
    mainSizer->Add(playerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    panel->SetSizer(mainSizer);
    
    // Create status bar
    m_statusBar = CreateStatusBar(1);
    m_statusBar->SetStatusText(wxT("Ready"));
    
    // Create file dialogs
    m_openDialog = new wxFileDialog(this, wxT("Open SBB File"), wxT(""), wxT(""),
                                    wxT("SBB files (*.sbb)|*.sbb|All files (*.*)|*.*"),
                                    wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    m_saveDialog = new wxFileDialog(this, wxT("Save SBB File"), wxT(""), wxT(""),
                                    wxT("SBB files (*.sbb)|*.sbb|All files (*.*)|*.*"),
                                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    // Initialize image list for machines
    // (Already initialized above before creating combo box)
    
    // Load settings
    LoadSettings();
    
    // Fix readonly combo box grey background on Windows
    #ifdef __WXMSW__
    m_modelCmbBx->Enable(true);
    m_frecuenciaCmbBx->Enable(true);
    m_muestrasCmbBx->Enable(true);
    #endif
    
    // Connect event handlers
    m_modelCmbBx->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& event) {
        // Update the model in the header when selection changes
        int sel = m_modelCmbBx->GetSelection();
        if (sel >= 0 && sel < (int)m_modelCmbBx->GetCount())
        {
            m_headSbb.model = sel + 1;
            // Explicitly set the value to ensure it displays on wxCB_READONLY
            wxString value = m_modelCmbBx->GetString(sel);
            m_modelCmbBx->SetValue(value);
        }
    });
    m_frecuenciaCmbBx->Bind(wxEVT_COMBOBOX, &MainFrame::OnFrequencyChange, this);
    m_wavBtn->Bind(wxEVT_BUTTON, &MainFrame::OnWav, this);
    m_mp3Btn->Bind(wxEVT_BUTTON, &MainFrame::OnMp3, this);
    m_tzxBtn->Bind(wxEVT_BUTTON, &MainFrame::OnTzx, this);
    m_playBtn->Bind(wxEVT_BUTTON, &MainFrame::OnPlayButton, this);
    
    // Connect menu items (using Bind for wxID_ANY items)
    m_convertMenu->Bind(wxEVT_MENU, &MainFrame::OnWav, this, m_wavMenuItem->GetId());
    m_convertMenu->Bind(wxEVT_MENU, &MainFrame::OnMp3, this, m_mp3MenuItem->GetId());
    m_convertMenu->Bind(wxEVT_MENU, &MainFrame::OnTzx, this, m_tzxMenuItem->GetId());
    
    m_toolsMenu->Bind(wxEVT_MENU, &MainFrame::OnBatchSbb, this, m_toolsMenu->FindItem(wxT("&Batch SBB...")));
    m_toolsMenu->Bind(wxEVT_MENU, &MainFrame::OnBatchWav, this, m_toolsMenu->FindItem(wxT("Batch &WAV...")));
    m_toolsMenu->Bind(wxEVT_MENU, &MainFrame::OnAdjustAudio, this, m_toolsMenu->FindItem(wxT("&Adjust Audio...")));
    m_toolsMenu->Bind(wxEVT_MENU, &MainFrame::OnEditBlocks, this, m_toolsMenu->FindItem(wxT("&Edit Blocks...")));
    m_toolsMenu->Bind(wxEVT_MENU, &MainFrame::OnWavPlayer, this, m_toolsMenu->FindItem(wxT("&WAV Player...")));
    
    m_helpMenu->Bind(wxEVT_MENU, &MainFrame::OnWiki, this, m_helpMenu->FindItem(wxT("&Wiki...")));
    
    // Initialize data
    ClearData();
    
    // Trigger machine change handler after frame is fully loaded
    // This ensures the machine selection from settings is properly initialized
    CallAfter([this]() {
        int sel = m_machineCmbBx->GetSelection();
        if (sel >= 0 && sel < 4)
        {
            wxCommandEvent dummyEvent;
            OnMachineChange(dummyEvent);
        }
    });
}

MainFrame::~MainFrame()
{
    SaveSettings();
    delete m_openDialog;
    delete m_saveDialog;
    if (m_machineImgLst)
    {
        delete m_machineImgLst;
        m_machineImgLst = nullptr;
    }
    if (m_waveformImgLst)
    {
        delete m_waveformImgLst;
        m_waveformImgLst = nullptr;
    }
}

void MainFrame::LoadSettings()
{
    // Paths
    wxString appPath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName appFile(appPath);
    wxString appDir = appFile.GetPath();
    
    m_iniFileName = appDir + wxFILE_SEP_PATH + wxT("otla.ini");
    
    wxFileConfig config(wxT("OTLA"), wxEmptyString, m_iniFileName);
    
    // Window position
    long x = config.ReadLong(wxT("/otla/Top"), -1);
    long y = config.ReadLong(wxT("/otla/Left"), -1);
    if (x != -1 && y != -1)
        SetPosition(wxPoint((int)x, (int)y));
    
    wxString defaultInputDir = appDir + wxFILE_SEP_PATH + wxT("sbb");
    wxString defaultOutputZX = appDir + wxFILE_SEP_PATH + wxT("sbb") + wxFILE_SEP_PATH + wxT("zx");
    wxString defaultOutputCPC = appDir + wxFILE_SEP_PATH + wxT("sbb") + wxFILE_SEP_PATH + wxT("cpc");
    wxString defaultOutputMSX = appDir + wxFILE_SEP_PATH + wxT("sbb") + wxFILE_SEP_PATH + wxT("msx");
    wxString defaultOutput81 = appDir + wxFILE_SEP_PATH + wxT("sbb") + wxFILE_SEP_PATH + wxT("81");
    wxString defaultToolDiskMgr = appDir + wxFILE_SEP_PATH + wxT("tools") + wxFILE_SEP_PATH + wxT("msx_diskmgr") + wxFILE_SEP_PATH + wxT("DISKMGR.EXE");
    wxString defaultToolCPC = wxT("WinApe.exe");
    wxString defaultOutputDir = appDir + wxFILE_SEP_PATH + wxT("output");
    wxString defaultLameExe = appDir + wxFILE_SEP_PATH + wxT("tools") + wxFILE_SEP_PATH + wxT("lame") + wxFILE_SEP_PATH + wxT("lame.exe");
    
    m_inputDir = config.Read(wxT("/otla/OpenPath"), defaultInputDir);
    m_outputZX = config.Read(wxT("/otlaBuilder/OutPutZX"), defaultOutputZX);
    m_outputCPC = config.Read(wxT("/otlaBuilder/OutPutCPC"), defaultOutputCPC);
    m_outputMSX = config.Read(wxT("/otlaBuilder/OutPutMSX"), defaultOutputMSX);
    m_output81 = config.Read(wxT("/otlaBuilder/OutPut81"), defaultOutput81);
    
    m_toolDiskMgr = config.Read(wxT("/otlaBuilder/msxdskmgr"), defaultToolDiskMgr);
    m_toolCPC = config.Read(wxT("/otlaBuilder/cpctool"), defaultToolCPC);

    // Sync with global variables
    ToolDiskMgr = m_toolDiskMgr;
    ToolCPC = m_toolCPC;
    
    long machineIndex = config.ReadLong(wxT("/otlaBuilder/machine"), 0L);
    
    m_outputDir = config.Read(wxT("/otlaPlayer/OutPutDir"), defaultOutputDir);
    m_outputInfo = config.ReadBool(wxT("/otlaPlayer/output_info"), false);
    m_viewHex = config.ReadBool(wxT("/otlaPlayer/view_hex"), false);
    
    // Store machine index - will be applied after frame is fully loaded
    // (SetSelection is done here, but OnMachineChange will be called via CallAfter)
    if (machineIndex >= 0 && machineIndex < 4)
    {
        m_machineCmbBx->SetSelection(machineIndex);
    }
    
    m_lameExe = config.Read(wxT("/otlaPlayer/lame"), defaultLameExe);
    m_bitRateMp3 = config.ReadLong(wxT("/otlaPlayer/bit_rate"), 256L);
    m_forceCubic = config.ReadBool(wxT("/otlaPlayer/force_cubic"), true);
    m_tsxMode = (int)config.ReadLong(wxT("/otlaPlayer/tsx_mode"), 0L);
    
    m_makeLoaderChkBx->SetValue(config.ReadLong(wxT("/otlaPlayer/make_loader"), 1L) != 0);
    m_accelerateChkBx->SetValue(config.ReadLong(wxT("/otlaPlayer/acelerate"), 0L) != 0);
    m_checkLoadErrorChkBx->SetValue(config.ReadLong(wxT("/otlaPlayer/checkloaderror"), 1L) != 0);
    m_releMotorChkBx->SetValue(config.ReadLong(wxT("/otlaPlayer/motor"), 1L) != 0);
    
    long freq = config.ReadLong(wxT("/otlaPlayer/freq"), 44100L);
    m_frecuenciaCmbBx->SetSelection(freq == 48000 ? 1 : 0);
    
    long spb = config.ReadLong(wxT("/otlaPlayer/s_p_b"), 1L);
    if (spb >= 0 && spb < (long)m_muestrasCmbBx->GetCount())
        m_muestrasCmbBx->SetSelection(spb);
    else
        m_muestrasCmbBx->SetSelection(1);
    
    long wavForm = config.ReadLong(wxT("/otlaPlayer/wav_form"), 1L);
    if (wavForm >= 0 && wavForm < (long)m_formaCmbBx->GetCount())
        m_formaCmbBx->SetSelection(wavForm);
    
    long invertPol = config.ReadLong(wxT("/otlaPlayer/invert_pol"), 1L);
    m_invertChkBx->SetValue(invertPol == -1);
    
    long pauseBlocks = config.ReadLong(wxT("/otlaPlayer/pause_blocks"), 500L);
    m_pausaEdt->SetValue(wxString::Format(wxT("%ld"), pauseBlocks));
    
    if (m_openDialog)
        m_openDialog->SetDirectory(m_inputDir);
}

void MainFrame::SaveSettings()
{
    wxFileConfig config(wxT("OTLA"), wxEmptyString, m_iniFileName);
    
    wxPoint pos = GetPosition();
    config.Write(wxT("/otla/Top"), (long)pos.y);
    config.Write(wxT("/otla/Left"), (long)pos.x);
    config.Write(wxT("/otla/OpenPath"), m_inputDir);
    
    if (m_openDialog)
        config.Write(wxT("/otlaBuilder/AddPath"), m_openDialog->GetDirectory());
    config.Write(wxT("/otlaBuilder/machine"), (long)m_machineCmbBx->GetSelection());
    
    config.Write(wxT("/otlaPlayer/make_loader"), m_makeLoaderChkBx->GetValue() ? 1L : 0L);
    config.Write(wxT("/otlaPlayer/acelerate"), m_accelerateChkBx->GetValue() ? 1L : 0L);
    config.Write(wxT("/otlaPlayer/checkloaderror"), m_checkLoadErrorChkBx->GetValue() ? 1L : 0L);
    config.Write(wxT("/otlaPlayer/motor"), m_releMotorChkBx->GetValue() ? 1L : 0L);
    
    int freq = m_frecuenciaCmbBx->GetSelection() == 1 ? 48000 : 44100;
    config.Write(wxT("/otlaPlayer/freq"), (long)freq);
    config.Write(wxT("/otlaPlayer/s_p_b"), (long)m_muestrasCmbBx->GetSelection());
    config.Write(wxT("/otlaPlayer/wav_form"), (long)m_formaCmbBx->GetSelection());
    config.Write(wxT("/otlaPlayer/invert_pol"), m_invertChkBx->GetValue() ? -1L : 1L);
    
    long pauseBlocks = 500;
    m_pausaEdt->GetValue().ToLong(&pauseBlocks);
    config.Write(wxT("/otlaPlayer/pause_blocks"), pauseBlocks);
    
    config.Flush();
}

void MainFrame::ClearData()
{
    m_addBtn->Enable(true);
    m_elFichero = wxT("");
    memset(m_blocksSbb, 0, sizeof(m_blocksSbb));
    memset(&m_headSbb, 0, sizeof(m_headSbb));
    
    m_machineCmbBx->Enable(true);
    m_modificado = false;
    
    m_playBtn->Enable(false);
    EnableConversionControls(false);
    
    m_statusBar->SetStatusText(wxT(""));
    
    m_blocksLV->DeleteAllItems();
}

void MainFrame::FillData()
{
    m_fileEdt->ChangeValue(m_elFichero);
    
    m_blocksLV->DeleteAllItems();
    int b = 0;
    while (m_blocksSbb[b].block_type != 0 && b < _MAX_SBB_BLOCKS_)
    {
        long index = m_blocksLV->InsertItem(m_blocksLV->GetItemCount(), wxString(m_blocksSbb[b].blockname, wxConvLocal));
        
        wxString sizeStr;
        if (m_viewHex)
            sizeStr = wxString::Format(wxT("$%04X"), m_blocksSbb[b].size);
        else
            sizeStr = wxString::Format(wxT("%u"), m_blocksSbb[b].size);
        m_blocksLV->SetItem(index, 1, sizeStr);
        
        m_blocksLV->SetItem(index, 2, wxString(TipoBloque(m_blocksSbb[b].block_type), wxConvLocal));
        
        wxString iniStr;
        if (m_viewHex)
            iniStr = wxString::Format(wxT("$%04X"), m_blocksSbb[b].ini);
        else
            iniStr = wxString::Format(wxT("%u"), m_blocksSbb[b].ini);
        m_blocksLV->SetItem(index, 3, iniStr);
        
        m_blocksLV->SetItem(index, 4, wxString::Format(wxT("%c"), m_blocksSbb[b].jump));
        
        wxString execStr;
        if (m_viewHex)
            execStr = wxString::Format(wxT("$%04X"), m_blocksSbb[b].exec);
        else
            execStr = wxString::Format(wxT("%u"), m_blocksSbb[b].exec);
        m_blocksLV->SetItem(index, 5, execStr);
        
        b++;
    }
    
    m_headSbb.n_blocks = b;
    
    ChangeMachine(m_headSbb.machine[0]);
    
    if (m_headSbb.model > 0)
        m_modelCmbBx->SetSelection((int)m_headSbb.model - 1);
    
    m_nameEdt->ChangeValue(wxString(m_headSbb.nombre, wxConvLocal));
    m_additionalInfoEdt->ChangeValue(wxString(m_headSbb.extra_info, wxConvLocal));
    m_blocksEdt->ChangeValue(wxString::Format(wxT("%d"), m_headSbb.n_blocks));
    m_originEdt->ChangeValue(wxString::Format(wxT("%c"), m_headSbb.origin));
    
    m_enableIntChkBx->SetValue((m_headSbb.ei_di & 1) != 0);
    
    wxString reubicaStr;
    if (m_viewHex)
        reubicaStr = wxString::Format(wxT("$%02X"), m_headSbb.locate);
    else
        reubicaStr = wxString::Format(wxT("%u"), m_headSbb.locate);
    m_reubicaEdt->ChangeValue(reubicaStr);

    wxString pokeStr;
    if (m_viewHex)
        pokeStr = wxString::Format(wxT("$%02X"), (int)m_headSbb.poke_ffff);
    else
        pokeStr = wxString::Format(wxT("%u"), (int)m_headSbb.poke_ffff);
    m_pokeEdt->ChangeValue(pokeStr);

    wxString clearStr;
    if (m_viewHex)
        clearStr = wxString::Format(wxT("$%04X"), m_headSbb.clear_sp);
    else
        clearStr = wxString::Format(wxT("%u"), m_headSbb.clear_sp);
    m_clearEdt->ChangeValue(clearStr);

    wxString usrStr;
    if (m_viewHex)
        usrStr = wxString::Format(wxT("$%04X"), m_headSbb.usr_pc);
    else
        usrStr = wxString::Format(wxT("%u"), m_headSbb.usr_pc);
    m_usrEdt->ChangeValue(usrStr);
}

void MainFrame::EnableConversionControls(bool enable)
{
    m_wavBtn->Enable(enable);
    m_mp3Btn->Enable(enable);
    m_tzxBtn->Enable(enable);
    m_wavMenuItem->Enable(enable);
    m_mp3MenuItem->Enable(enable);
    m_tzxMenuItem->Enable(enable);
}

void MainFrame::ChangeMachine(char machine)
{
    m_locateLbl->Enable(false);
    m_reubicaEdt->Enable(false);
    m_pokeLbl->Enable(false);
    m_pokeEdt->Enable(false);
    
    m_clearLbl->Enable(true);
    m_clearEdt->Enable(true);
    m_usrLbl->Enable(true);
    m_usrEdt->Enable(true);
    
    switch (machine)
    {
        case 'Z':
            m_machineCmbBx->SetSelection(0);
            m_openDialog->SetWildcard(wxT("ZX files (*.tap;*.tzx;*.z80;*.sna;*.scr;*.dsk;*.sbb)|*.tap;*.tzx;*.z80;*.sna;*.scr;*.dsk;*.sbb;*.hst"));
            m_outputSBB = m_outputZX;
            m_nameEdt->SetMaxLength(10);
            m_clearLbl->SetLabel(wxT("CLEAR:"));
            m_usrLbl->SetLabel(wxT("USR:"));
            m_checkLoadErrorChkBx->Enable(true);
            m_enableIntChkBx->Enable(true);
            m_releMotorChkBx->Enable(false);
            m_tzxBtn->SetLabel(wxT("SBB => TZX"));
            m_tzxMenuItem->SetItemLabel(wxT("&TZX..."));
            break;
        case 'C':
            m_machineCmbBx->SetSelection(1);
            m_openDialog->SetWildcard(wxT("CPC files (*.sna;*.bin;*.cdt;*.dsk;*.sbb)|*.sna;*.bin;*.dsk;*.cdt;*.sbb;*.hst"));
            m_outputSBB = m_outputCPC;
            m_nameEdt->SetMaxLength(16);
            m_locateLbl->Enable(true);
            m_reubicaEdt->Enable(true);
            m_clearLbl->SetLabel(wxT("SP:"));
            m_usrLbl->SetLabel(wxT("JP:"));
            m_checkLoadErrorChkBx->Enable(true);
            m_enableIntChkBx->Enable(true);
            m_releMotorChkBx->Enable(true);
            m_tzxBtn->SetLabel(wxT("SBB => CDT"));
            m_tzxMenuItem->SetItemLabel(wxT("&CDT..."));
            break;
        case 'M':
            m_machineCmbBx->SetSelection(2);
            m_openDialog->SetWildcard(wxT("MSX files (*.bin;*.cas;*.dsk;*.rom;*.sbb)|*.bin;*.rom;*.cas;*.dsk;*.sbb;*.hst"));
            m_outputSBB = m_outputMSX;
            m_nameEdt->SetMaxLength(6);
            m_locateLbl->Enable(true);
            m_reubicaEdt->Enable(true);
            m_pokeLbl->Enable(true);
            m_pokeEdt->Enable(true);
            m_clearLbl->SetLabel(wxT("CLEAR:"));
            m_usrLbl->SetLabel(wxT("JP:"));
            m_checkLoadErrorChkBx->Enable(true);
            m_enableIntChkBx->Enable(true);
            m_releMotorChkBx->Enable(true);
            m_tzxBtn->SetLabel(wxT("SBB => TSX"));
            m_tzxMenuItem->SetItemLabel(wxT("&TSX..."));
            break;
        case '8':
            m_machineCmbBx->SetSelection(3);
            m_openDialog->SetWildcard(wxT("81 files (*.p;*.z81;*.sbb)|*.p;*.z81;*.sbb;*.hst"));
            m_outputSBB = m_output81;
            m_nameEdt->SetMaxLength(2);
            m_clearLbl->SetLabel(wxT("SP:"));
            m_usrLbl->SetLabel(wxT("USR:"));
            m_clearLbl->Enable(false);
            m_clearEdt->Enable(false);
            m_usrLbl->Enable(false);
            m_usrEdt->Enable(false);
            m_checkLoadErrorChkBx->Enable(false);
            m_enableIntChkBx->Enable(false);
            m_releMotorChkBx->Enable(false);
            m_tzxBtn->SetLabel(wxT("SBB => TZX"));
            m_tzxMenuItem->SetItemLabel(wxT("&TZX..."));
            break;
        default:
            m_machineCmbBx->SetSelection(-1);
            wxMessageBox(wxT("Error: Unknown machine"), wxT("Error"), wxOK | wxICON_ERROR, this);
            break;
    }
    
    OnFrequencyChange(wxCommandEvent()); // Trigger frequency change handler
    Layout(); // Reflow after label text changes (e.g. "SP:" vs "CLEAR:", "JP:" vs "USR:")
}

void MainFrame::UpdateHeader()
{
    // Update machine string from combo box
    int machineIndex = m_machineCmbBx->GetSelection();
    const char* machines[] = {"ZXS", "CPC", "MSX", "81-"};
    if (machineIndex >= 0 && machineIndex < 4)
    {
        strncpy(m_headSbb.machine, machines[machineIndex], sizeof(m_headSbb.machine));
    }
    
    m_headSbb.model = m_modelCmbBx->GetSelection() + 1;
    
    wxString name = m_nameEdt->GetValue();
    strncpy(m_headSbb.nombre, name.mb_str(wxConvLocal), sizeof(m_headSbb.nombre) - 1);
    m_headSbb.nombre[sizeof(m_headSbb.nombre) - 1] = '\0';
    
    wxString extra = m_additionalInfoEdt->GetValue();
    strncpy(m_headSbb.extra_info, extra.mb_str(wxConvLocal), sizeof(m_headSbb.extra_info) - 1);
    m_headSbb.extra_info[sizeof(m_headSbb.extra_info) - 1] = '\0';
    
    m_headSbb.ei_di = m_enableIntChkBx->GetValue() ? 1 : 0;
    
    wxString reubicaStr = m_reubicaEdt->GetValue();
    m_headSbb.locate = inputDecHex(reubicaStr.mb_str(wxConvLocal));
    
    wxString clearStr = m_clearEdt->GetValue();
    m_headSbb.clear_sp = inputDecHex(clearStr.mb_str(wxConvLocal));
    
    wxString usrStr = m_usrEdt->GetValue();
    m_headSbb.usr_pc = inputDecHex(usrStr.mb_str(wxConvLocal));
    
    wxString pokeStr = m_pokeEdt->GetValue();
    m_headSbb.poke_ffff = inputDecHex(pokeStr.mb_str(wxConvLocal));
}

void MainFrame::SetWavVars()
{
    // Set WAV generation variables from UI
    // These are global variables in wav2.cpp and loaders.cpp
    // (declared in wav2.h and loaders.h)
    
    make_loader = m_makeLoaderChkBx->GetValue() ? 1 : 0;
    acelerar = m_accelerateChkBx->GetValue() ? 1 : 0;
    checkloaderror = m_checkLoadErrorChkBx->GetValue() ? 1 : 0;
    motor = m_releMotorChkBx->GetValue() ? 1 : 0;
    
    long pauseVal = 500;
    m_pausaEdt->GetValue().ToLong(&pauseVal);
    pausa_ms = (int)pauseVal;
    
    invert_pol = m_invertChkBx->GetValue() ? -1 : 1;
    forma_onda = m_formaCmbBx->GetSelection();
    
    freq_muestreo = m_frecuenciaCmbBx->GetSelection() == 1 ? 48000 : 44100;
    
    // Set samples per bit based on machine and selection
    // Constants are defined in wav2.h
    int spbIndex = m_muestrasCmbBx->GetSelection();
    if (m_headSbb.machine[0] == '8')
    {
        // ZX81 specific values (different mapping)
        switch (spbIndex)
        {
            case 0: s_p_b = _3_50; break;
            case 1: s_p_b = _2_75; break;
            case 2: s_p_b = _2_50; break;
            case 3: s_p_b = _1_75; break;
            default: s_p_b = _3_50; break;
        }
    }
    else
    {
        // Standard values (sorted descending s/b, bps ascending)
        switch (spbIndex)
        {
            case 0: s_p_b = _4_00; break;
            case 1: s_p_b = _3_50; break;
            case 2: s_p_b = _3_00; break;
            case 3: s_p_b = _2_75; break;  // 2.75 now at index 3
            case 4: s_p_b = _2_50; break;  // 2.5  now at index 4
            case 5: s_p_b = _2_25; break;
            case 6: s_p_b = _1_75; break;
            default: break;
        }
    }
}

wxString MainFrame::MakeNameWav(const wxString& filename)
{
    float samples;
    if (m_headSbb.machine[0] == '8')
    {
        wxString spbText = m_muestrasCmbBx->GetStringSelection();
        long spbVal = 6;
        spbText.BeforeFirst(wxT(' ')).ToLong(&spbVal);
        samples = (float)spbVal;
    }
    else
    {
        samples = s_p_b / 4.0f;
    }

    wxString extrainfo;
    if (m_outputInfo)
    {
        extrainfo = wxString::Format(wxT("_%i_%.2f_w%i"),
                                     freq_muestreo / 1000,
                                     samples,
                                     forma_onda);
        extrainfo += (invert_pol == -1) ? wxT("n") : wxT("p");
    }

    return m_outputDir + wxFILE_SEP_PATH +
           wxFileName(filename).GetName() + extrainfo + wxT(".wav");
}

int MainFrame::AutoSaveSBB()
{
    if (!m_modificado)
        return 0;
    
    if (m_nuevo)
    {
        if (wxFileExists(m_elFichero))
        {
            int res = wxMessageBox(m_elFichero + wxT(" already exists.\r\nDo you want to overwrite it?"),
                                   wxT("Warning"), wxYES_NO | wxICON_QUESTION, this);
            if (res == wxNO)
                return -1;
        }
    }
    
    OnSave(wxCommandEvent());
    m_nuevo = false;
    return 0;
}

int MainFrame::OpenFileSBB(const wxString& fileToOpen)
{
    wxString filePath = fileToOpen;
    if (filePath.IsEmpty())
    {
        // Force SBB-only filter for File>Open. ChangeMachine() may have set
        // m_openDialog's wildcard to a machine-specific source-file list,
        // but only .sbb files are valid here.
        wxString prevWildcard = m_openDialog->GetWildcard();
        m_openDialog->SetWildcard(wxT("SBB files (*.sbb)|*.sbb"));
        int dlgResult = m_openDialog->ShowModal();
        m_openDialog->SetWildcard(prevWildcard);
        if (dlgResult == wxID_CANCEL)
            return -1;
        filePath = m_openDialog->GetPath();
    }
    
    // Load SBB file
    wxString mbPath = filePath.mb_str(wxConvLocal);
    int res = load_sbb_file(mbPath.c_str(), &m_headSbb, m_blocksSbb);
    
    if (res == 0)
    {
        m_elFichero = filePath;
        m_inputDir = wxFileName(filePath).GetPath();
        FillData();
        m_modificado = false;
        m_nuevo = false;
        m_machineCmbBx->Enable(false);
        if (m_headSbb.machine[0] == '8')
            m_addBtn->Enable(false);
        m_playBtn->Enable(true);
        EnableConversionControls(true);
        m_statusBar->SetStatusText(wxT("File loaded"));
        return 0;
    }
    else
    {
        wxMessageBox(wxString::Format(wxT("Error loading file: %s"), error_file(res)), 
                     wxT("Error"), wxOK | wxICON_ERROR, this);
        return res;
    }
}

void MainFrame::LameProcess(const wxString& wavIn, char maq, const char* title)
{
    if (m_lameExe.IsEmpty())
    {
        wxMessageBox(wxT("LAME executable not configured"), wxT("Error"), wxOK | wxICON_ERROR, this);
        return;
    }

    wxString titleStr(title, wxConvLocal);
    titleStr.Trim().Trim(false);
    if (titleStr.IsEmpty())
        titleStr = wxFileName(m_elFichero).GetName();

    wxString machineName(NombreMachine(maq), wxConvLocal);
    wxString titleFull = machineName + wxT(", ") + titleStr;

    wxString artist = wxT("O.T.L.A. ") + wxString(version_SBB, wxConvLocal);

    wxString album = m_additionalInfoEdt->GetValue();
    album.Trim().Trim(false);

    wxString spbText = m_muestrasCmbBx->GetStringSelection();
    wxString formaText = m_formaCmbBx->GetStringSelection();
    wxString comments = wxT("s/b: ") + spbText + wxT(" waveform:") + formaText;

    wxString params = wxString::Format(
        wxT(" \"%s\" -m m -f -b %i --tt \"%s\" --ta \"%s\" --tc \"%s\""),
        wavIn.c_str(),
        m_bitRateMp3,
        titleFull.c_str(),
        artist.c_str(),
        comments.c_str());

    if (!album.IsEmpty())
        params += wxString::Format(wxT(" --tl \"%s\""), album.c_str());

    wxString command = m_lameExe + params;

    long pid = wxExecute(command, wxEXEC_ASYNC);
    if (pid == 0)
    {
        wxMessageBox(wxT("Error trying lame.exe"),
                     wxT("Error"), wxOK | wxICON_ERROR, this);
    }
}

// Event handlers
void MainFrame::OnNew(wxCommandEvent& event)
{
    ClearData();
    m_elFichero = wxT("new.SBB");

    // ClearData() zeroes m_headSbb; restore machine from the active combo entry
    // so ChangeMachine() inside FillData() gets a valid character.
    int sel = m_machineCmbBx->GetSelection();
    const char* machines[] = {"ZXS", "CPC", "MSX", "81-"};
    if (sel >= 0 && sel < 4)
        strncpy(m_headSbb.machine, machines[sel], sizeof(m_headSbb.machine));

    FillData();
    m_modificado = false;
    m_nuevo = true;
}

void MainFrame::OnOpen(wxCommandEvent& event)
{
    OpenFileSBB(wxT(""));
}

void MainFrame::OnSave(wxCommandEvent& event)
{
    UpdateHeader();
    
    m_headSbb.n_blocks = 0;
    
    // Count blocks and set jump/exec
    while (m_blocksSbb[m_headSbb.n_blocks].block_type != 0 && m_headSbb.n_blocks < _MAX_SBB_BLOCKS_)
    {
        if (m_blocksSbb[m_headSbb.n_blocks + 1].block_type == 0)
        {
            // Last block
            if (m_blocksSbb[m_headSbb.n_blocks].exec)
                execjump(&m_blocksSbb[m_headSbb.n_blocks], 0xc3); // JP
            else
                execjump(&m_blocksSbb[m_headSbb.n_blocks], 0xc9); // RET
        }
        else
        {
            // Not last block
            if (m_blocksSbb[m_headSbb.n_blocks].exec)
                execjump(&m_blocksSbb[m_headSbb.n_blocks], 0xcd); // CALL
            else
                execjump(&m_blocksSbb[m_headSbb.n_blocks], 0x01); // LD BC,xxxx
        }
        
        // Check for overlap with loader (CPC/MSX)
        if (m_headSbb.machine[0] == 'C' || m_headSbb.machine[0] == 'M')
        {
            int ini = m_blocksSbb[m_headSbb.n_blocks].ini;
            int last = ini + m_blocksSbb[m_headSbb.n_blocks].size;
            int loaderAddr = m_headSbb.locate * 0x100;
            
            if (ini < loaderAddr && last > loaderAddr)
            {
                wxString msg = wxString::Format(wxT("Last bytes of block %d override address of loading routine located at $%04X\r\nThis block will be truncated when converting to wav"),
                                                m_headSbb.n_blocks + 1, loaderAddr);
                wxMessageBox(msg, wxT("Warning"), wxOK | wxICON_WARNING, this);
            }
            if (ini > loaderAddr && ini < loaderAddr + 0x100)
            {
                wxString msg = wxString::Format(wxT("Start address of block %d overrides address of loading routine located at $%04X"),
                                                m_headSbb.n_blocks + 1, loaderAddr);
                wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
                return;
            }
        }
        
        m_headSbb.n_blocks++;
    }
    
    wxString mbPath = m_elFichero.mb_str(wxConvLocal);
    int res = save_sbb_file(mbPath.c_str(), &m_headSbb, m_blocksSbb);
    
    if (res != 0)
    {
        wxString msg = m_elFichero + wxT("\r\n");
        msg += wxString(error_file(res), wxConvLocal);
        wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
    }
    else
    {
        m_modificado = false;
        m_statusBar->SetStatusText(wxT("File saved"));
        m_inputDir = m_outputSBB;
    }
}

void MainFrame::OnSaveAs(wxCommandEvent& event)
{
    if (m_saveDialog->ShowModal() == wxID_OK)
    {
        m_elFichero = m_saveDialog->GetPath();
        OnSave(event);
    }
}

void MainFrame::OnQuit(wxCommandEvent& event)
{
    Close();
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    AboutDialog dlg(this);
    dlg.ShowModal();
}

void MainFrame::OnWiki(wxCommandEvent& event)
{
    wxLaunchDefaultBrowser(wxT("http://code.google.com/p/otla/w/list"));
}

void MainFrame::OnMachineChange(wxCommandEvent& event)
{
    int sel = m_machineCmbBx->GetSelection();
    const char* machines[] = {"ZXS", "CPC", "MSX", "81-"};
    
    m_modelCmbBx->Clear();
    
    if (sel >= 0 && sel < 4)
    {
        strncpy(m_headSbb.machine, machines[sel], sizeof(m_headSbb.machine) - 1);
        m_headSbb.machine[sizeof(m_headSbb.machine) - 1] = '\0';
        
        // Set default locate value
        switch (sel)
        {
            case 0: m_headSbb.locate = 0xff; break; // ZX
            case 1: m_headSbb.locate = 0xff; break; // CPC
            case 2: m_headSbb.locate = 0xf4; break; // MSX
            case 3: m_headSbb.locate = 0x00; break; // ZX81
        }
        
        // Add model options
        for (int i = 1; i <= 3; i++)
        {
            const char* model = modelo(m_headSbb.machine[0], i);
            if (model && strlen(model) > 0)
                m_modelCmbBx->Append(wxString(model, wxConvLocal));
        }
        
        // Select the current model if valid
        if (m_headSbb.model > 0 && m_headSbb.model <= 3)
        {
            m_modelCmbBx->SetSelection(m_headSbb.model - 1);
        }
        else if (m_modelCmbBx->GetCount() > 0)
        {
            m_modelCmbBx->SetSelection(0);
            m_headSbb.model = 1;
        }
        
        ChangeMachine(m_headSbb.machine[0]);
        
        wxString reubicaStr;
        if (m_viewHex)
            reubicaStr = wxString::Format(wxT("$%02X"), m_headSbb.locate);
        else
            reubicaStr = wxString::Format(wxT("%u"), m_headSbb.locate);
        m_reubicaEdt->SetValue(reubicaStr);
    }
}

void MainFrame::OnAddButton(wxCommandEvent& event)
{
    wxFileDialog dlg(this, wxT("Open File"), m_inputDir, wxT(""),
                     wxT("All supported files|*.tap;*.tzx;*.z80;*.sna;*.scr;*.dsk;*.sbb;*.hst;*.cas;*.cdt;*.bin;*.rom;*.p;*.z81|"
                          "ZX files (*.tap;*.tzx;*.z80;*.sna;*.scr;*.dsk;*.sbb;*.hst)|*.tap;*.tzx;*.z80;*.sna;*.scr;*.dsk;*.sbb;*.hst|"
                          "CPC files (*.sna;*.bin;*.cdt;*.dsk;*.sbb;*.hst)|*.sna;*.bin;*.dsk;*.cdt;*.sbb;*.hst|"
                          "MSX files (*.cas;*.bin;*.rom;*.sbb;*.hst)|*.cas;*.bin;*.rom;*.sbb;*.hst|"
                          "81 files (*.p;*.z81;*.sbb;*.hst)|*.p;*.z81;*.sbb;*.hst|"
                          "All files (*.*)|*.*"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dlg.ShowModal() == wxID_OK)
    {
        wxString filename = dlg.GetPath();
        m_inputDir = wxFileName(filename).GetPath();
        
        // Ensure n_blocks is valid and within bounds
        if (m_headSbb.n_blocks < 0)
            m_headSbb.n_blocks = 0;
        if (m_headSbb.n_blocks >= _MAX_SBB_BLOCKS_)
        {
            wxMessageBox(wxT("Maximum number of blocks reached"), wxT("Error"), wxOK | wxICON_ERROR);
            return;
        }
        
        // Count actual blocks to ensure we have the correct count
        int actualBlocks = 0;
        while (actualBlocks < _MAX_SBB_BLOCKS_ && m_blocksSbb[actualBlocks].block_type != 0)
            actualBlocks++;
        
        // Use actual block count instead of n_blocks
        int insertPos = actualBlocks;
        if (insertPos >= _MAX_SBB_BLOCKS_)
        {
            wxMessageBox(wxT("Maximum number of blocks reached"), wxT("Error"), wxOK | wxICON_ERROR);
            return;
        }
        
        InputDialog* inputDlg = new InputDialog(this);
        inputDlg->head_sbb = m_headSbb;  // Copy current header
        inputDlg->enheaxadecimal = m_viewHex;
        inputDlg->res_blocks_sbb = &m_blocksSbb[insertPos];  // Point to position after existing blocks
        
        if (inputDlg->Abre(filename) == wxID_OK)
        {
            m_modificado = true;
            
            // If no output file set yet, create one based on input filename.
            // Also rebuild when m_nuevo is true: OnNew() seeds m_elFichero with
            // "new.SBB" as a placeholder (no path, generic name) — adding the
            // first source file should replace that with the per-machine
            // output dir + the source file's basename.
            if (m_elFichero.IsEmpty() || m_nuevo)
            {
                char machine = m_headSbb.machine[0];
                wxString outputDir;
                switch (machine)
                {
                    case 'Z': outputDir = m_outputZX; break;
                    case 'C': outputDir = m_outputCPC; break;
                    case 'M': outputDir = m_outputMSX; break;
                    case '8': outputDir = m_output81; break;
                }
                m_elFichero = outputDir + wxFILE_SEP_PATH + wxFileName(filename).GetName() + wxT(".sbb");
                // Keep m_nuevo true: AutoSaveSBB still needs to prompt the
                // user before overwriting an existing file on disk with the
                // freshly-derived name.
            }
            
            // Merge header information
            ts_sbb_header newHead = inputDlg->head_sbb;
            
            // If machine type wasn't set, set it from the loaded file
            if (m_headSbb.machine[0] == 0 && newHead.machine[0] != 0)
            {
                m_headSbb.machine[0] = newHead.machine[0];
                m_headSbb.machine[1] = newHead.machine[1];
            }
            
            if (newHead.origin == _Snapshot4ZX48_ || 
                newHead.origin == _Snapshot4ZX128_ || 
                newHead.origin == _Snapshot4CPC_)
            {
                m_headSbb.origin = newHead.origin;
                m_headSbb.ei_di &= 0xfe;
            }
            else if (m_headSbb.origin == 0)
            {
                m_headSbb.origin = newHead.origin;
                m_headSbb.ei_di |= 0x01;
            }
            
            if (m_headSbb.model == 0)
                m_headSbb.model = newHead.model;
            if (m_headSbb.nombre[0] == 0)
                strncpy(m_headSbb.nombre, newHead.nombre, sizeof(m_headSbb.nombre));

            // CLEAR/USR/POKE: always adopt the dialog's values. The user just
            // edited them in the InputDialog, so they should win over whatever
            // EVT_TEXT-driven UpdateHeader() already copied into m_headSbb.
            m_headSbb.usr_pc    = newHead.usr_pc;
            m_headSbb.clear_sp  = newHead.clear_sp;
            m_headSbb.poke_ffff = newHead.poke_ffff;
            
            // Count new blocks
            int newBlocks = 0;
            while (inputDlg->res_blocks_sbb[newBlocks].block_type != 0)
                newBlocks++;
            
            m_headSbb.n_blocks += newBlocks;
            
            FillData();
            ChangeMachine(m_headSbb.machine[0]);
            
            // Enable conversion buttons
            m_playBtn->Enable(true);
            EnableConversionControls(true);
            
            if (m_headSbb.machine[0] == '8')
                m_addBtn->Enable(false);
        }
        
        // Clean up the dialog
        inputDlg->Destroy();
    }
}

void MainFrame::OnFrequencyChange(wxCommandEvent& event)
{
    int currentSel = m_muestrasCmbBx->GetSelection();
    
    m_muestrasCmbBx->Clear();
    
    bool is48000 = m_frecuenciaCmbBx->GetSelection() == 1;
    char machine = m_headSbb.machine[0];
    
    if (is48000)
    {
        m_muestrasCmbBx->Append(wxT("4    (12000 bps)"));
        m_muestrasCmbBx->Append(wxT("3.5  (13600 bps)"));
        m_muestrasCmbBx->Append(wxT("3    (16200 bps)"));

        if (machine == 'Z')
        {
            // ZX Spectrum: only 4 speed options; clamp any slower selection to index 3
            if (currentSel >= 3)
                currentSel = 3;
            m_muestrasCmbBx->Append(wxT("2.5  (19200 bps)"));
        }
        else if (machine != '8')
        {
            // MSX/CPC: sorted descending s/b (bps ascending); 2.75 before 2.5
            m_muestrasCmbBx->Append(wxT("2.75 (17454 bps)"));
            m_muestrasCmbBx->Append(wxT("2.5  (19200 bps)"));
            m_muestrasCmbBx->Append(wxT("2.25 (21333 bps)"));
            m_muestrasCmbBx->Append(wxT("1.75 (27428 bps)"));
        }

        if (machine == '8')
        {
            m_muestrasCmbBx->Clear();
            m_muestrasCmbBx->Append(wxT("6 ( 8000 bps)"));
            m_muestrasCmbBx->Append(wxT("5 ( 9600 bps)"));
            m_muestrasCmbBx->Append(wxT("4 (12000 bps)"));
            m_muestrasCmbBx->Append(wxT("3 (16200 bps)"));
        }
    }
    else
    {
        m_muestrasCmbBx->Append(wxT("4    (11025 bps)"));
        m_muestrasCmbBx->Append(wxT("3.5  (12600 bps)"));
        m_muestrasCmbBx->Append(wxT("3    (14700 bps)"));

        if (machine == 'Z')
        {
            if (currentSel >= 3)
                currentSel = 3;
            m_muestrasCmbBx->Append(wxT("2.5  (17640 bps)"));
        }
        else if (machine != '8')
        {
            m_muestrasCmbBx->Append(wxT("2.75 (16036 bps)"));
            m_muestrasCmbBx->Append(wxT("2.5  (17640 bps)"));
            m_muestrasCmbBx->Append(wxT("2.25 (19600 bps)"));
            m_muestrasCmbBx->Append(wxT("1.75 (25200 bps)"));
        }

        if (machine == '8')
        {
            m_muestrasCmbBx->Clear();
            m_muestrasCmbBx->Append(wxT("6 ( 7350 bps)"));
            m_muestrasCmbBx->Append(wxT("5 ( 8820 bps)"));
            m_muestrasCmbBx->Append(wxT("4 (11025 bps)"));
            m_muestrasCmbBx->Append(wxT("3 (14700 bps)"));
        }
    }
    
    if (currentSel >= 0 && currentSel < (int)m_muestrasCmbBx->GetCount())
        m_muestrasCmbBx->SetSelection(currentSel);
    else if (m_muestrasCmbBx->GetCount() > 0)
        m_muestrasCmbBx->SetSelection(0);
}

void MainFrame::OnNameChange(wxCommandEvent& event)
{
    UpdateHeader();
    m_modificado = true;
}

void MainFrame::OnWav(wxCommandEvent& event)
{
    SetWavVars();
    m_wavFile = wxT("");
    
    if (AutoSaveSBB() != 0)
        return;
    
    m_wavFile = MakeNameWav(m_elFichero);
    
    wxString mbSbb = m_elFichero.mb_str(wxConvLocal);
    wxString mbWav = m_wavFile.mb_str(wxConvLocal);
    
    int res = sbb2wav(mbSbb.c_str(), mbWav.c_str());
    
    if (res)
    {
        wxString msg = m_elFichero + wxT(" =>\r\n") + m_wavFile + wxT("\r\n");
        msg += wxString(error_file(res), wxConvLocal);
        wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
        m_wavFile = wxT("");
    }
    else
    {
        double duration = dura() + 0.05;
        wxString msg = wxString::Format(wxT("%s \r\n Total time: %.1f"), m_wavFile.c_str(), duration);
        if (m_showInfo)
            wxMessageBox(msg, wxT("Information"), wxOK | wxICON_INFORMATION, this);
    }
}

void MainFrame::OnMp3(wxCommandEvent& event)
{
    int tempForma = m_formaCmbBx->GetSelection();
    if (m_forceCubic)
        m_formaCmbBx->SetSelection(1);
    
    m_showInfo = false;
    OnWav(event);
    m_showInfo = true;
    
    m_formaCmbBx->SetSelection(tempForma);
    
    if (!m_wavFile.IsEmpty())
    {
        wxString name = m_nameEdt->GetValue();
        LameProcess(m_wavFile, m_headSbb.machine[0], name.mb_str(wxConvLocal));
    }
}

void MainFrame::OnTzx(wxCommandEvent& event)
{
    SetWavVars();
    if (AutoSaveSBB() != 0)
        return;
    
    wxString tempWav = m_outputDir + wxFILE_SEP_PATH + wxT("temp.wav");
    wxString mbSbb = m_elFichero.mb_str(wxConvLocal);
    wxString mbTempWav = tempWav.mb_str(wxConvLocal);
    
    int res = sbb2wav(mbSbb.c_str(), mbTempWav.c_str());
    
    if (res)
    {
        wxString msg = m_elFichero + wxT(" =>\r\n") + tempWav + wxT("\r\n");
        msg += wxString(error_file(res), wxConvLocal);
        wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
        return;
    }
    
    wxFileName fn(m_elFichero);
    wxString final = m_outputDir + wxFILE_SEP_PATH + fn.GetName();

    char machine = m_headSbb.machine[0];
    if (machine == 'C')
        final += wxT(".cdt");
    else if (machine == 'M')
        final += wxT(".tsx");
    else
        final += wxT(".tzx");

    if (machine == 'M')
    {
        // MSX: direct SBB -> TSX (TZX 1.21 + MSX #4B bootstrap +
        // #15 or #19 veloz payload depending on user setting). No WAV
        // intermediate, no external tools.
        wxScopedCharBuffer sbbBuf = m_elFichero.utf8_str();
        wxScopedCharBuffer tsxBuf = final.utf8_str();

        sbb2tsx_options opts;
        opts.mode = (sbb2tsx_mode)m_tsxMode;
        opts.description = "Generated by O.T.L.A.";

        int rc = sbb2tsx(sbbBuf.data(), tsxBuf.data(), &opts);
        if (rc != 0)
        {
            wxString msg = wxString::Format(
                wxT("SBB -> TSX conversion failed (code %d)\n\n%s\n=> %s"),
                rc, m_elFichero.c_str(), final.c_str());
            wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
        }
    }
    else
    {
        // ZX / CPC / ZX81: internal wav2tzx
        float samples;
        if (machine == '8')
        {
            wxString spbText = m_muestrasCmbBx->GetStringSelection();
            long spbVal = 6;
            spbText.BeforeFirst(' ').ToLong(&spbVal);
            samples = (float)spbVal;
        }
        else
        {
            samples = s_p_b / 4.0f;
        }

        wxString params = wxString::Format(wxT("O.T.L.A. Samples/bit:%.2f, %iHz"), samples, freq_muestreo);

        wxString mbFinal = final.mb_str(wxConvLocal);
        wxString mbParams = params.mb_str(wxConvLocal);
        res = wav2tzx(mbTempWav.c_str(), mbFinal.c_str(), mbParams.c_str());

        if (res)
        {
            wxString msg = final + wxT("\r\n");
            msg += wxString(error_file(res), wxConvLocal);
            wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
        }
    }

    wxRemoveFile(tempWav);
}

void MainFrame::OnPlayButton(wxCommandEvent& event)
{
    m_playBtn->Enable(false);
    
    m_statusBar->SetStatusText(wxT("Creating wav..."));
    wxYield(); // Process messages
    
    wxString final = m_outputDir + wxFILE_SEP_PATH + wxT("play.wav");
    
    SetWavVars();
    
    if (AutoSaveSBB() == 0)
    {
        wxString mbSbb = m_elFichero.mb_str(wxConvLocal);
        wxString mbWav = final.mb_str(wxConvLocal);
        
        int res = sbb2wav(mbSbb.c_str(), mbWav.c_str());
        
        if (res)
        {
            wxString msg = m_elFichero + wxT("\r\n");
            msg += wxString(error_file(res), wxConvLocal);
            wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
        }
        else
        {
            m_statusBar->SetStatusText(wxT("Playing..."));
            wxYield();
            
            WavMonitorDialog dlg(this);
            if (dlg.OpenWaveFile(final) == 0)
            {
                dlg.Play();
                dlg.ShowModal();
            }
        }
    }
    
    m_playBtn->Enable(true);
    m_statusBar->SetStatusText(wxT(""));
}

void MainFrame::OnSettings(wxCommandEvent& event)
{
    SettingsDialog dlg(this);
    
    // Set current values
    dlg.SetDirZX(m_outputZX);
    dlg.SetDirCPC(m_outputCPC);
    dlg.SetDirMSX(m_outputMSX);
    dlg.SetDir81(m_output81);
    dlg.SetDiskMgr(m_toolDiskMgr);
    dlg.SetCPCTool(m_toolCPC);
    dlg.SetWavDir(m_outputDir);
    dlg.SetLame(m_lameExe);
    dlg.SetHexadecimal(m_viewHex);
    dlg.SetForceCubic(m_forceCubic);
    dlg.SetInfo(m_outputInfo);
    dlg.SetBitRate(m_bitRateMp3);
    dlg.SetTsxMode(m_tsxMode);
    
    if (dlg.ShowModal() == wxID_OK)
    {
        // Get new values
        m_outputZX = dlg.GetDirZX();
        m_outputCPC = dlg.GetDirCPC();
        m_outputMSX = dlg.GetDirMSX();
        m_output81 = dlg.GetDir81();
        m_toolDiskMgr = dlg.GetDiskMgr();
        m_toolCPC = dlg.GetCPCTool();

        // Sync with global variables
        ToolDiskMgr = m_toolDiskMgr;
        ToolCPC = m_toolCPC;
        m_outputDir = dlg.GetWavDir();
        m_lameExe = dlg.GetLame();
        m_viewHex = dlg.GetHexadecimal();
        m_forceCubic = dlg.GetForceCubic();
        m_outputInfo = dlg.GetInfo();
        m_bitRateMp3 = dlg.GetBitRate();
        m_tsxMode = dlg.GetTsxMode();

        // Save to config
        wxFileConfig config(wxT("OTLA"), wxEmptyString, m_iniFileName);
        config.Write(wxT("/otlaBuilder/OutPutZX"), m_outputZX);
        config.Write(wxT("/otlaBuilder/OutPutCPC"), m_outputCPC);
        config.Write(wxT("/otlaBuilder/OutPutMSX"), m_outputMSX);
        config.Write(wxT("/otlaBuilder/OutPut81"), m_output81);
        config.Write(wxT("/otlaBuilder/msxdskmgr"), m_toolDiskMgr);
        config.Write(wxT("/otlaBuilder/cpctool"), m_toolCPC);
        config.Write(wxT("/otlaPlayer/OutPutDir"), m_outputDir);
        config.Write(wxT("/otlaPlayer/lame"), m_lameExe);
        config.Write(wxT("/otlaPlayer/view_hex"), m_viewHex);
        config.Write(wxT("/otlaPlayer/force_cubic"), m_forceCubic);
        config.Write(wxT("/otlaPlayer/output_info"), m_outputInfo);
        config.Write(wxT("/otlaPlayer/bit_rate"), (long)m_bitRateMp3);
        config.Write(wxT("/otlaPlayer/tsx_mode"), (long)m_tsxMode);
        config.Flush();
        
        UpdateHeader();
        FillData();
    }
}

void MainFrame::OnBatchSbb(wxCommandEvent& event)
{
    char machine = m_headSbb.machine[0];
    if (machine == 0)
    {
        wxMessageBox(wxT("Please select a machine type first"), wxT("Error"), wxOK | wxICON_ERROR);
        return;
    }
    
    BatchDialog dlg(this);
    dlg.Batch(machine, 1);  // Type 1 = SBB conversion
}

void MainFrame::OnBatchWav(wxCommandEvent& event)
{
    char machine = m_headSbb.machine[0];
    if (machine == 0)
    {
        wxMessageBox(wxT("Please select a machine type first"), wxT("Error"), wxOK | wxICON_ERROR);
        return;
    }
    
    BatchDialog dlg(this);
    dlg.Batch(machine, 2);  // Type 2 = WAV conversion
}

void MainFrame::OnAdjustAudio(wxCommandEvent& event)
{
    char machine = m_headSbb.machine[0];
    if (machine == 0)
    {
        wxMessageBox(wxT("Please select a machine type first"), wxT("Error"), wxOK | wxICON_ERROR);
        return;
    }
    
    AdjustDialog dlg(this);
    dlg.SetMachine(machine);
    dlg.SetFrequency(m_frecuenciaCmbBx->GetSelection());

    wxString outputDir = m_outputDir;
    if (outputDir.IsEmpty())
        outputDir = wxStandardPaths::Get().GetDocumentsDir();
    dlg.SetOutputDir(outputDir);
    
    dlg.ShowModal();
}

void MainFrame::OnEditBlocks(wxCommandEvent& event)
{
    InputDialog* dlg = new InputDialog(this);
    dlg->head_sbb = m_headSbb;
    dlg->enheaxadecimal = m_viewHex;
    dlg->res_blocks_sbb = m_blocksSbb;
    
    if (dlg->Edit(&m_headSbb, m_blocksSbb) == wxID_OK)
    {
        // Count blocks
        int count = 0;
        while (dlg->res_blocks_sbb[count].block_type != 0)
            count++;
        
        // Copy back the edited blocks
        for (int i = 0; i < count && i < _MAX_SBB_BLOCKS_; i++)
        {
            m_blocksSbb[i] = dlg->res_blocks_sbb[i];
        }
        m_blocksSbb[count].block_type = 0;
        
        m_headSbb.n_blocks = count;
        m_headSbb = dlg->head_sbb;
        
        FillData();
        m_modificado = true;
    }
    
    dlg->Destroy();
}

void MainFrame::OnWavPlayer(wxCommandEvent& event)
{
    WavMonitorDialog dlg(this);
    dlg.ShowModal();
}

void MainFrame::CreateWaveformIcons()
{
    // Create programmatic waveform icons (24x24 pixels)
    const int w = 24;
    const int h = 24;
    const int centerY = h / 2;
    
    // Square wave
    {
        wxImage img(w, h);
        img.SetRGB(wxRect(0, 0, w, h), 255, 255, 255); // White background
        unsigned char* data = img.GetData();
        
        for (int x = 0; x < w; x++)
        {
            int y = (x < w/2) ? centerY - 6 : centerY + 6;
            if (y >= 0 && y < h)
            {
                int idx = (y * w + x) * 3;
                data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0; // Black
            }
        }
        // Add vertical transitions
        for (int y = centerY - 6; y <= centerY + 6; y++)
        {
            if (y >= 0 && y < h)
            {
                int idx = (y * w + w/2) * 3;
                data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0;
            }
        }
        
        wxBitmap bmp(img);
        m_waveformImgLst->Add(bmp);
        m_formaCmbBx->Append(wxT("Square"), bmp);
    }
    
    // Cubic (smooth curve - sine wave)
    {
        wxImage img(w, h);
        img.SetRGB(wxRect(0, 0, w, h), 255, 255, 255);
        unsigned char* data = img.GetData();
        
        for (int x = 0; x < w; x++)
        {
            double t = (double)x / w * 2.0 * 3.14159265; // One cycle
            int y = centerY - (int)(6 * sin(t));
            if (y >= 0 && y < h)
            {
                int idx = (y * w + x) * 3;
                data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0;
            }
        }
        
        wxBitmap bmp(img);
        m_waveformImgLst->Add(bmp);
        m_formaCmbBx->Append(wxT("Cubic"), bmp);
    }
    
    // SqrSin (square-ish sine)
    {
        wxImage img(w, h);
        img.SetRGB(wxRect(0, 0, w, h), 255, 255, 255);
        unsigned char* data = img.GetData();
        
        for (int x = 0; x < w; x++)
        {
            double t = (double)x / w * 2.0 * 3.14159265;
            double val = sin(t);
            // Make it more square-ish
            val = val > 0 ? sqrt(val) : -sqrt(-val);
            int y = centerY - (int)(6 * val);
            if (y >= 0 && y < h)
            {
                int idx = (y * w + x) * 3;
                data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0;
            }
        }
        
        wxBitmap bmp(img);
        m_waveformImgLst->Add(bmp);
        m_formaCmbBx->Append(wxT("SqrSin"), bmp);
    }
    
    // Shaw (triangle wave)
    {
        wxImage img(w, h);
        img.SetRGB(wxRect(0, 0, w, h), 255, 255, 255);
        unsigned char* data = img.GetData();
        
        for (int x = 0; x < w; x++)
        {
            int y;
            if (x < w/2)
                y = centerY - 6 + (x * 12 / (w/2));
            else
                y = centerY + 6 - ((x - w/2) * 12 / (w/2));
            
            if (y >= 0 && y < h)
            {
                int idx = (y * w + x) * 3;
                data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0;
            }
        }
        
        wxBitmap bmp(img);
        m_waveformImgLst->Add(bmp);
        m_formaCmbBx->Append(wxT("Shaw"), bmp);
    }
    
    // E=cte (ramp/sawtooth)
    {
        wxImage img(w, h);
        img.SetRGB(wxRect(0, 0, w, h), 255, 255, 255);
        unsigned char* data = img.GetData();
        
        for (int x = 0; x < w; x++)
        {
            int y = centerY - 6 + (x * 12 / w);
            if (y >= 0 && y < h)
            {
                int idx = (y * w + x) * 3;
                data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0;
            }
        }
        
        wxBitmap bmp(img);
        m_waveformImgLst->Add(bmp);
        m_formaCmbBx->Append(wxT("E=cte"), bmp);
    }
    
    // Delta (pulse/spike)
    {
        wxImage img(w, h);
        img.SetRGB(wxRect(0, 0, w, h), 255, 255, 255);
        unsigned char* data = img.GetData();
        
        // Horizontal center line
        for (int x = 0; x < w; x++)
        {
            int idx = (centerY * w + x) * 3;
            data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0;
        }
        
        // Vertical spikes at quarter points
        for (int spike = 0; spike < 2; spike++)
        {
            int xPos = (spike + 1) * w / 3;
            for (int y = centerY - 8; y <= centerY + 8; y++)
            {
                if (y >= 0 && y < h)
                {
                    int idx = (y * w + xPos) * 3;
                    data[idx] = 0; data[idx+1] = 0; data[idx+2] = 0;
                }
            }
        }
        
        wxBitmap bmp(img);
        m_waveformImgLst->Add(bmp);
        m_formaCmbBx->Append(wxT("Delta"), bmp);
    }
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    SaveSettings();
    if (m_machineImgLst)
    {
        delete m_machineImgLst;
        m_machineImgLst = nullptr;
    }
    event.Skip();
}

void MainFrame::LoadMachineIcons()
{
    if (!m_machineImgLst)
        return;
    
#if HAVE_EMBEDDED_BMP
    // Load from embedded resources
    wxImage images[4];
    
    // Load in order: ZX Spectrum, CPC, MSX, ZX81
    images[0] = LoadZxspectrumBmp();
    images[1] = LoadAmstredBmp();
    images[2] = LoadMsxBmp();
    
    // Try zx81 first, fallback to 81
    images[3] = LoadZx81Bmp();
    if (!images[3].IsOk())
    {
        images[3] = LoadBmp81Bmp();  // "81" becomes "Bmp81" in C++ identifier
    }
    
    for (size_t i = 0; i < 4; i++)
    {
        if (images[i].IsOk())
        {
            // Resize to WIDE format 140x46 for prominent display (like reference design)
            if (images[i].GetWidth() != 140 || images[i].GetHeight() != 46)
            {
                images[i] = images[i].Scale(140, 46, wxIMAGE_QUALITY_HIGH);
            }
            wxBitmap bmp(images[i]);
            if (bmp.IsOk())
            {
                m_machineImgLst->Add(bmp);
            }
        }
    }
#else
    // Fallback: Load from files (for when embedded resources are not available)
    // Get the executable path to find the bmp directory
    wxString appPath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName appFile(appPath);
    wxString appDir = appFile.GetPath();
    
    // Try multiple possible locations for BMP files
    wxArrayString bmpPaths;
    bmpPaths.Add(appDir + wxFILE_SEP_PATH + wxT("bmp"));
    bmpPaths.Add(appDir + wxFILE_SEP_PATH + wxT("..") + wxFILE_SEP_PATH + wxT("source") + wxFILE_SEP_PATH + wxT("bmp"));
    bmpPaths.Add(appDir + wxFILE_SEP_PATH + wxT("..") + wxFILE_SEP_PATH + wxT("..") + wxFILE_SEP_PATH + wxT("source") + wxFILE_SEP_PATH + wxT("bmp"));
    
    wxString bmpDir;
    for (size_t i = 0; i < bmpPaths.GetCount(); i++)
    {
        if (wxDir::Exists(bmpPaths[i]))
        {
            bmpDir = bmpPaths[i];
            break;
        }
    }
    
    if (bmpDir.IsEmpty())
    {
        // Try to find bmp directory relative to source
        wxString sourceBmp = wxT("..") + wxString(wxFILE_SEP_PATH) + wxT("source") + wxString(wxFILE_SEP_PATH) + wxT("bmp");
        if (wxDir::Exists(sourceBmp))
            bmpDir = sourceBmp;
    }
    
    if (bmpDir.IsEmpty())
        return; // Can't find bmp directory
    
    // Load wide machine logos in order: ZX Spectrum, CPC, MSX, ZX81
    wxArrayString iconFiles;
    iconFiles.Add(bmpDir + wxFILE_SEP_PATH + wxT("zxspectrum.bmp"));
    iconFiles.Add(bmpDir + wxFILE_SEP_PATH + wxT("amstred.bmp"));
    iconFiles.Add(bmpDir + wxFILE_SEP_PATH + wxT("msx.bmp"));
    iconFiles.Add(bmpDir + wxFILE_SEP_PATH + wxT("zx81.bmp"));
    
    // Try alternative name for ZX81
    if (!wxFile::Exists(iconFiles[3]))
        iconFiles[3] = bmpDir + wxFILE_SEP_PATH + wxT("81.bmp");
    
    for (size_t i = 0; i < iconFiles.GetCount(); i++)
    {
        if (wxFile::Exists(iconFiles[i]))
        {
            wxImage img;
            if (img.LoadFile(iconFiles[i], wxBITMAP_TYPE_BMP))
            {
                // Resize to WIDE format 140x46 for prominent display (like reference design)
                if (img.GetWidth() != 140 || img.GetHeight() != 46)
                {
                    img = img.Scale(140, 46, wxIMAGE_QUALITY_HIGH);
                }
                wxBitmap bmp(img);
                if (bmp.IsOk())
                {
                    m_machineImgLst->Add(bmp);
                }
            }
        }
    }
#endif
}

