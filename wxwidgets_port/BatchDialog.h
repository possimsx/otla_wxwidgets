#ifndef BATCHDIALOG_H
#define BATCHDIALOG_H

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include "../source/sbb.h"
#include "../source/zxfiles.h"
#include "../source/cpcfiles.h"
#include "../source/msxfiles.h"
#include "../source/81files.h"

class MainFrame;

class BatchDialog : public wxDialog
{
public:
    BatchDialog(wxWindow* parent);
    
    void Batch(char m, int t);
    
private:
    void OnInputDir(wxCommandEvent& event);
    void OnOutputDir(wxCommandEvent& event);
    void OnProceed(wxCommandEvent& event);
    
    void ConvertToSbb();
    void ConvertToWav();
    int BuildSbb(const wxString& file);
    int N_Files(const wxString& mask);
    
    wxTextCtrl* m_memo;
    wxTextCtrl* m_inputDirEdt;
    wxTextCtrl* m_outputDirEdt;
    wxComboBox* m_extensionCmbBx;
    wxCheckBox* m_makeMp3ChkBx;
    wxGauge* m_progressBar;
    wxStaticText* m_label2;
    
    char m_maquina;
    int m_tipo;
    int m_m_y_e;
    
};

#endif

