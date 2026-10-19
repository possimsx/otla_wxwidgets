#ifndef MODIFYDIALOG_H
#define MODIFYDIALOG_H

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include "../source/sbb.h"

class ModifyDialog : public wxDialog
{
public:
    ModifyDialog(wxWindow* parent);
    ~ModifyDialog();
    
    void SetBlock(ts_sbb_block* block);
    void GetBlock(ts_sbb_block* block);
    
private:
    wxTextCtrl* m_nameEdt;
    wxTextCtrl* m_startEdt;
    wxTextCtrl* m_sizeEdt;
    wxTextCtrl* m_execEdt;
    wxTextCtrl* m_memo;
    
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

#endif

