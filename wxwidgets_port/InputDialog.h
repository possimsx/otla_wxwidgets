#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/combobox.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include "../source/sbb.h"
#include "../source/zxfiles.h"
#include "../source/cpcfiles.h"
#include "../source/msxfiles.h"
#include "../source/81files.h"

// Forward declarations
class ModifyDialog;

class InputDialog : public wxDialog
{
public:
    InputDialog(wxWindow* parent);
    ~InputDialog();
    
    int Abre(const wxString& fichero);
    int Edit(ts_sbb_header* in_head_sbb, ts_sbb_block* in_blocks_sbb);
    
    ts_sbb_header head_sbb;
    ts_sbb_block* res_blocks_sbb;
    bool enheaxadecimal;
    
private:
    void FillData();
    int ROMMSX(const char* file);
    int Estate(ts_sbb_block* block);
    
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnModify(wxCommandEvent& event);
    void OnSeeData(wxCommandEvent& event);
    void OnConvertBasic(wxCommandEvent& event);
    void OnRestore(wxCommandEvent& event);
    void OnViewHex(wxCommandEvent& event);
    void OnPopupMenu(wxContextMenuEvent& event);
    void OnListItemSelected(wxListEvent& event);
    
    wxListCtrl* m_listView;
    wxTextCtrl* m_memo;
    wxStaticText* m_clearLbl;
    wxStaticText* m_usrLbl;
    wxTextCtrl* m_clearEdt;
    wxComboBox* m_usrCmbBx;
    wxMenu* m_popupMenu;
    
    // Menu item IDs
    int m_menuIdModify;
    int m_menuIdSeeData;
    int m_menuIdConvertBasic;
    int m_menuIdRestore;
    int m_menuIdViewHex;
    
    ts_sbb_block m_blocksSbb[16];

    wxString m_toolDiskMgr;
    wxString m_toolCPC;

    wxDECLARE_EVENT_TABLE();
};

#endif

