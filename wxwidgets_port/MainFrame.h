#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/combobox.h>
#include <wx/bmpcbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/filedlg.h>
#include <wx/config.h>
#include <wx/frame.h>
#include <wx/statbox.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/filename.h>

#include "../source/sbb.h"
#include "../source/funciones.h"
#include "../source/loaders.h"
#include "../source/wav2.h"

// Forward declarations
class AboutDialog;
class InputDialog;
class ModifyDialog;
class SettingsDialog;
class DirDialog;
class BatchDialog;
class AdjustDialog;
class WavMonitorDialog;

class MainFrame : public wxFrame
{
public:
    MainFrame();
    ~MainFrame();
    
    // Public method to open file (called from main.cpp)
    int OpenFileSBB(const wxString& fileToOpen);
    
private:
    // Menu handlers
    void OnNew(wxCommandEvent& event);
    void OnNewSave(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnWav(wxCommandEvent& event);
    void OnMp3(wxCommandEvent& event);
    void OnTzx(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    void OnBatchSbb(wxCommandEvent& event);
    void OnBatchWav(wxCommandEvent& event);
    void OnAdjustAudio(wxCommandEvent& event);
    void OnEditBlocks(wxCommandEvent& event);
    void OnWavPlayer(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnWiki(wxCommandEvent& event);
    
    // Control handlers
    void OnMachineChange(wxCommandEvent& event);
    void OnAddButton(wxCommandEvent& event);
    void OnFrequencyChange(wxCommandEvent& event);
    void OnNameChange(wxCommandEvent& event);
    void OnPlayButton(wxCommandEvent& event);
    
    // Window handlers
    void OnClose(wxCloseEvent& event);
    
    // Helper methods
    void LoadSettings();
    void SaveSettings();
    void ClearData();
    void FillData();
    void UpdateHeader();
    void ChangeMachine(char machine);
    void SetWavVars();
    void EnableConversionControls(bool enable);
    wxString MakeNameWav(const wxString& filename);
    int AutoSaveSBB();
    void LoadMachineIcons();
    void CreateWaveformIcons();
    
public:
    void LameProcess(const wxString& wavIn, char maq, const char* title);
    
    // Menu
    wxMenuBar* m_menuBar;
    wxMenu* m_fileMenu;
    wxMenu* m_convertMenu;
    wxMenu* m_toolsMenu;
    wxMenu* m_helpMenu;
    wxMenuItem* m_wavMenuItem;
    wxMenuItem* m_mp3MenuItem;
    wxMenuItem* m_tzxMenuItem;
    
    // Builder group
    wxStaticBox* m_builderGB;
    wxListCtrl* m_blocksLV;
    wxStaticBox* m_headerGB;
    wxStaticText* m_nameLbl;
    wxStaticText* m_clearLbl;
    wxStaticText* m_usrLbl;
    wxStaticText* m_pokeLbl;
    wxStaticText* m_additionalLbl;
    wxStaticText* m_blocksLbl;
    wxStaticText* m_locateLbl;
    wxStaticText* m_modelLbl;
    wxTextCtrl* m_nameEdt;
    wxTextCtrl* m_clearEdt;
    wxTextCtrl* m_usrEdt;
    wxTextCtrl* m_pokeEdt;
    wxTextCtrl* m_additionalInfoEdt;
    wxButton* m_addBtn;
    wxTextCtrl* m_blocksEdt;
    wxTextCtrl* m_originEdt;
    wxTextCtrl* m_reubicaEdt;
    wxComboBox* m_modelCmbBx;
    wxBitmapComboBox* m_machineCmbBx;
    wxImageList* m_machineImgLst;
    wxImageList* m_waveformImgLst;
    
    // Player group
    wxStaticBox* m_playerGB;
    wxStaticBox* m_wavGB;
    wxCheckBox* m_invertChkBx;
    wxBitmapComboBox* m_formaCmbBx;
    wxComboBox* m_frecuenciaCmbBx;
    wxComboBox* m_muestrasCmbBx;
    wxTextCtrl* m_pausaEdt;
    
    // Loader group
    wxStaticBox* m_loaderGB;
    wxCheckBox* m_checkLoadErrorChkBx;
    wxCheckBox* m_releMotorChkBx;
    wxCheckBox* m_makeLoaderChkBx;
    wxCheckBox* m_accelerateChkBx;
    wxCheckBox* m_enableIntChkBx;
    
    // Buttons
    wxButton* m_wavBtn;
    wxButton* m_mp3Btn;
    wxButton* m_tzxBtn;
    wxButton* m_playBtn;
    
    // File dialog
    wxFileDialog* m_openDialog;
    wxFileDialog* m_saveDialog;
    
    // Status bar
    wxStatusBar* m_statusBar;
    
    // File edit
    wxTextCtrl* m_fileEdt;
    
    // Data
    wxString m_elFichero;
    bool m_nuevo;
    bool m_modificado;
    wxString m_wavFile;
    
    // Settings
    bool m_viewHex;
    int m_bitRateMp3;
    bool m_forceCubic;
    wxString m_lameExe;
    wxString m_outputDir;
    bool m_outputInfo;
    bool m_showInfo;
    int  m_tsxMode;      // 0 = #4B+#15 (default), 1 = #4B+#19, 2 = legacy single #15
    
    // Paths
    wxString m_iniFileName;
    wxString m_outputZX;
    wxString m_outputCPC;
    wxString m_outputMSX;
    wxString m_output81;
    wxString m_outputSBB;
    wxString m_inputDir;
    
    // Tools
    wxString m_toolDiskMgr;
    wxString m_toolCPC;
    
    // SBB data
    ts_sbb_header m_headSbb;
    ts_sbb_block m_blocksSbb[_MAX_SBB_BLOCKS_ + 1];
    
    wxDECLARE_EVENT_TABLE();
};

#endif // MAINFRAME_H


