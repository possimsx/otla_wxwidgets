#ifndef DIRDIALOG_H
#define DIRDIALOG_H

#include <wx/dialog.h>
#include <wx/dirctrl.h>
#include <wx/sizer.h>

class DirDialog : public wxDialog
{
public:
    DirDialog(wxWindow* parent);
    ~DirDialog();
    
    void SetInitialDirectory(const wxString& dir);
    wxString GetDirectory() const;
    
private:
    wxGenericDirCtrl* m_dirCtrl;
    
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

#endif

