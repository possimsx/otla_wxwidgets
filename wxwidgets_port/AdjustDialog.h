#ifndef ADJUSTDIALOG_H
#define ADJUSTDIALOG_H

#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include "../source/signaltest.h"
#include "../source/wav2.h"

class WavMonitorDialog;

class AdjustDialog : public wxDialog
{
public:
    AdjustDialog(wxWindow* parent);
    
    void SetMachine(char maquina);
    void SetOutputDir(const wxString& dir);
    void SetFrequency(int idx);
    
private:
    void OnMakeProgram(wxCommandEvent& event);
    void OnMakeSignal(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnFrequencyChange(wxCommandEvent& event);
    void OnPlayProgram(wxCommandEvent& event);
    void OnPlaySignal(wxCommandEvent& event);
    
    wxComboBox* m_frecuenciaCmbBx;
    wxComboBox* m_formaCmbBx;
    wxComboBox* m_testSignalCmbBx;
    wxCheckBox* m_invertChkBx;
    
    wxString m_programWav;
    wxString m_outputDir;
    wxString m_signalName;
    char m_maquina;
    
};

#endif

