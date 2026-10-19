#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/radiobox.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

class SettingsDialog : public wxDialog
{
public:
    SettingsDialog(wxWindow* parent);
    ~SettingsDialog();
    
    // Getters
    wxString GetDirZX() const { return m_dirZXEdt->GetValue(); }
    wxString GetDirCPC() const { return m_dirCPCEdt->GetValue(); }
    wxString GetDirMSX() const { return m_dirMSXEdt->GetValue(); }
    wxString GetDir81() const { return m_dir81Edt->GetValue(); }
    wxString GetDiskMgr() const { return m_diskMgrEdt->GetValue(); }
    wxString GetCPCTool() const { return m_cpcToolEdt->GetValue(); }
    wxString GetWavDir() const { return m_wavDirEdt->GetValue(); }
    wxString GetLame() const { return m_lameEdt->GetValue(); }
    bool GetHexadecimal() const { return m_hexadecimalChkBx->GetValue(); }
    bool GetForceCubic() const { return m_forceCubicChkBx->GetValue(); }
    bool GetInfo() const { return m_infoChkBx->GetValue(); }
    int  GetTsxMode() const;
    int GetBitRate() const;
    
    // Setters
    void SetDirZX(const wxString& dir) { m_dirZXEdt->SetValue(dir); }
    void SetDirCPC(const wxString& dir) { m_dirCPCEdt->SetValue(dir); }
    void SetDirMSX(const wxString& dir) { m_dirMSXEdt->SetValue(dir); }
    void SetDir81(const wxString& dir) { m_dir81Edt->SetValue(dir); }
    void SetDiskMgr(const wxString& path) { m_diskMgrEdt->SetValue(path); }
    void SetCPCTool(const wxString& path) { m_cpcToolEdt->SetValue(path); }
    void SetWavDir(const wxString& dir) { m_wavDirEdt->SetValue(dir); }
    void SetLame(const wxString& path) { m_lameEdt->SetValue(path); }
    void SetHexadecimal(bool hex) { m_hexadecimalChkBx->SetValue(hex); }
    void SetForceCubic(bool cubic) { m_forceCubicChkBx->SetValue(cubic); }
    void SetInfo(bool info) { m_infoChkBx->SetValue(info); }
    void SetTsxMode(int v);
    void SetBitRate(int rate);
    
private:
    void OnDirZXBtn(wxCommandEvent& event);
    void OnDirCPCBtn(wxCommandEvent& event);
    void OnDirMSXBtn(wxCommandEvent& event);
    void OnDir81Btn(wxCommandEvent& event);
    void OnDiskMgrBtn(wxCommandEvent& event);
    void OnCPCToolBtn(wxCommandEvent& event);
    void OnWavDirBtn(wxCommandEvent& event);
    void OnLameBtn(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    wxTextCtrl* m_dirZXEdt;
    wxTextCtrl* m_dirCPCEdt;
    wxTextCtrl* m_dirMSXEdt;
    wxTextCtrl* m_dir81Edt;
    wxTextCtrl* m_diskMgrEdt;
    wxTextCtrl* m_cpcToolEdt;
    wxTextCtrl* m_wavDirEdt;
    wxTextCtrl* m_lameEdt;
    wxComboBox* m_bitRateCmbBx;
    wxCheckBox* m_hexadecimalChkBx;
    wxCheckBox* m_forceCubicChkBx;
    wxCheckBox* m_infoChkBx;
    wxRadioBox* m_tsxModeRadio;
    
    wxDECLARE_EVENT_TABLE();
};

#endif

