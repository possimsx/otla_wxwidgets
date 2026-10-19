#include "InputDialog.h"
#include "ModifyDialog.h"
#include "../source/funciones.h"
#include "../source/zxfiles.h"
#include "../source/cpcfiles.h"
#include "../source/msxfiles.h"
#include "../source/81files.h"
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/utils.h>
#include <wx/dir.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// Tool paths - defined in MainFrame.cpp
extern wxString ToolDiskMgr;
extern wxString ToolCPC;

// Internal MSX ROM -> SBB block(s) conversion.
//
// Replaces the legacy external msxr2b.exe pipeline (copy ROM to temp.rom,
// shell out, scan for temp*.bin, re-read each one via lee_msx_bin). The
// conversion now populates blocks directly in memory and mimics the BIN
// layout msxr2b produced: a 16K ROM becomes one block (load address based
// on the ROM's init vector slot), and a 32K ROM is split into two 16K
// blocks at $4000 and $8000. exec is set to the ROM header's init address
// on the last block so the OTLA loader jumps to it after loading completes.
int lee_msx_rom(const char* filename, ts_sbb_header* head, ts_sbb_block* blocks_rom, const char* /*rom2bin*/)
{
    int res = load_file(filename);
    if (res != 0)
        return res;

    // Accept 8K, 16K or 32K ROMs (matches msxr2b's supported sizes)
    if (tamanyo != 0x2000 && tamanyo != 0x4000 && tamanyo != 0x8000)
        return E_FORMATO;

    // Validate MSX cartridge signature "AB" at the load page boundary.
    // The signature can appear at offset 0 (page-aligned ROM) or at the
    // ROM's page offset; checking offset 0 covers the standard case.
    if (bufferfile[0] != 'A' || bufferfile[1] != 'B')
        return E_FORMATO;

    unsigned short init_addr = bufferfile[2] | (bufferfile[3] << 8);

    // Preserve user-configured locate / poke_ffff. poke_ffff == 0 keeps the
    // loader's Universal POKE (see start_loading in velozloader_msx6.asm) and
    // installs the mapper-init CALL — exactly what we want for ROM loading.
    // Overwriting with 170 (as the old external-tool path did) would bypass
    // both, leaving an MSX2+ without correctly mapped RAM.
    unsigned char saved_locate    = head->locate;
    unsigned char saved_poke_ffff = head->poke_ffff;
    unsigned short saved_clear_sp = head->clear_sp;
    unsigned short saved_usr_pc   = head->usr_pc;

    memset(head, 0, sizeof(*head));
    strcpy(head->machine, "MSX");
    head->model = -1;
    head->ei_di = 1;
    asigna_nombre(head, filename);
    head->origin = _rom_;
    head->locate    = saved_locate;
    head->poke_ffff = saved_poke_ffff;
    head->clear_sp  = saved_clear_sp;
    head->usr_pc    = saved_usr_pc;

    char short_name[6] = {0};
    wxString fn = wxFileName(filename).GetName();
    strncpy(short_name, fn.mb_str(wxConvLocal), 5);

    int n_blocks;
    if (tamanyo == 0x8000)
    {
        // 32K ROM: load first half at $4000-$7FFF, second half at $8000-$BFFF.
        // The OTLA MSX loader's mapper-init routine sets pages 1 and 2 to
        // distinct mapper segments in page 3's slot, so both halves land in
        // contiguous RAM. exec on the last block jumps to the ROM init vector.
        ts_sbb_block* b1 = &blocks_rom[0];
        b1->block_type = _msx_bin_;
        b1->ini = 0x4000;
        b1->size = 0x4000;
        b1->exec = 0;
        b1->param3 = 0;
        copy_data(b1, bufferfile);
        execjump(b1, 0x01);
        snprintf(b1->blockname, sizeof(b1->blockname), "1%s", short_name);

        ts_sbb_block* b2 = &blocks_rom[1];
        b2->block_type = _msx_bin_;
        b2->ini = 0x8000;
        b2->size = 0x4000;
        b2->exec = init_addr;
        b2->param3 = 0;
        copy_data(b2, bufferfile + 0x4000);
        execjump(b2, init_addr ? 0xc3 : 0xc9);
        snprintf(b2->blockname, sizeof(b2->blockname), "2%s", short_name);

        n_blocks = 2;
    }
    else
    {
        // 8K/16K ROM: load address picked from the ROM's init vector slot.
        unsigned short load_addr = (init_addr >= 0x8000 && init_addr < 0xC000) ? 0x8000 : 0x4000;

        ts_sbb_block* b1 = &blocks_rom[0];
        b1->block_type = _msx_bin_;
        b1->ini = load_addr;
        b1->size = (unsigned short)tamanyo;
        b1->exec = init_addr;
        b1->param3 = 0;
        copy_data(b1, bufferfile);
        execjump(b1, init_addr ? 0xc3 : 0xc9);
        snprintf(b1->blockname, sizeof(b1->blockname), "1%s", short_name);

        n_blocks = 1;
    }

    blocks_rom[n_blocks].block_type = 0;
    head->n_blocks = (unsigned char)n_blocks;

    return 0;
}

wxBEGIN_EVENT_TABLE(InputDialog, wxDialog)
    EVT_BUTTON(wxID_OK, InputDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, InputDialog::OnCancel)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, InputDialog::OnListItemSelected)
    EVT_CONTEXT_MENU(InputDialog::OnPopupMenu)
wxEND_EVENT_TABLE()

InputDialog::InputDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Input Blocks"), wxDefaultPosition, wxSize(660, 440),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    res_blocks_sbb = nullptr;
    enheaxadecimal = false;
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // List view for blocks. EnableCheckBoxes adds a per-row checkbox so the
    // user can pick which blocks get added to the SBB (matches the original
    // VCL form's "Checkboxes = True" on ListView1).
    m_listView = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 220),
                                wxLC_REPORT | wxLC_SINGLE_SEL);
    m_listView->EnableCheckBoxes(true);
    m_listView->InsertColumn(0, wxT("Block name"), wxLIST_FORMAT_LEFT, 120);
    m_listView->InsertColumn(1, wxT("Size"), wxLIST_FORMAT_RIGHT, 60);
    m_listView->InsertColumn(2, wxT("Type"), wxLIST_FORMAT_LEFT, 100);
    m_listView->InsertColumn(3, wxT("Start"), wxLIST_FORMAT_LEFT, 70);
    m_listView->InsertColumn(4, wxT("Exec"), wxLIST_FORMAT_LEFT, 70);
    m_listView->InsertColumn(5, wxT("Param3"), wxLIST_FORMAT_LEFT, 70);
    m_listView->InsertColumn(6, wxT("CLEAR"), wxLIST_FORMAT_LEFT, 70);
    m_listView->InsertColumn(7, wxT("USR"), wxLIST_FORMAT_LEFT, 70);
    mainSizer->Add(m_listView, 1, wxEXPAND | wxALL, 5);
    
    // Clear/USR fields
    wxBoxSizer* fieldsSizer = new wxBoxSizer(wxHORIZONTAL);
    m_clearLbl = new wxStaticText(this, wxID_ANY, wxT("CLEAR:"));
    m_clearEdt = new wxTextCtrl(this, wxID_ANY);
    fieldsSizer->Add(m_clearLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    fieldsSizer->Add(m_clearEdt, 0, wxEXPAND | wxRIGHT, 10);
    
    m_usrLbl = new wxStaticText(this, wxID_ANY, wxT("USR:"));
    m_usrCmbBx = new wxComboBox(this, wxID_ANY);
    fieldsSizer->Add(m_usrLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    fieldsSizer->Add(m_usrCmbBx, 0, wxEXPAND);
    mainSizer->Add(fieldsSizer, 0, wxEXPAND | wxALL, 5);
    
    // Memo for info — monospace font so hex dumps and memory views line up,
    // matching the Borland version's "Terminal" font on Memo1.
    m_memo = new wxTextCtrl(this, wxID_ANY, wxT(""), wxDefaultPosition, wxSize(-1, 100),
                            wxTE_MULTILINE | wxTE_READONLY);
    m_memo->SetFont(wxFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_TELETYPE,
                           wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    mainSizer->Add(m_memo, 0, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxStdDialogButtonSizer* btnSizer = wxDialog::CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);
    
    // Popup menu
    m_popupMenu = new wxMenu();
    m_menuIdModify = wxNewId();
    m_menuIdSeeData = wxNewId();
    m_menuIdConvertBasic = wxNewId();
    m_menuIdRestore = wxNewId();
    m_menuIdViewHex = wxNewId();
    
    m_popupMenu->Append(m_menuIdModify, wxT("Modify"));
    m_popupMenu->Append(m_menuIdSeeData, wxT("See data"));
    m_popupMenu->Append(m_menuIdConvertBasic, wxT("Convert Basic"));
    m_popupMenu->Append(m_menuIdRestore, wxT("Restore"));
    m_popupMenu->AppendSeparator();
    m_popupMenu->Append(m_menuIdViewHex, wxT("View decimal/hexadecimal"));
    
    m_popupMenu->Bind(wxEVT_MENU, &InputDialog::OnModify, this, m_menuIdModify);
    m_popupMenu->Bind(wxEVT_MENU, &InputDialog::OnSeeData, this, m_menuIdSeeData);
    m_popupMenu->Bind(wxEVT_MENU, &InputDialog::OnConvertBasic, this, m_menuIdConvertBasic);
    m_popupMenu->Bind(wxEVT_MENU, &InputDialog::OnRestore, this, m_menuIdRestore);
    m_popupMenu->Bind(wxEVT_MENU, &InputDialog::OnViewHex, this, m_menuIdViewHex);
    
    SetSizer(mainSizer);
    Centre();
}

InputDialog::~InputDialog()
{
    delete m_popupMenu;
}

int InputDialog::Estate(ts_sbb_block* block)
{
    switch (block->block_type)
    {
        case _test_: return 0;
        case _zx_program_: return 3;
        case _zx_char_array_: return 2;
        case _zx_num_array_: return 2;
        case _zx_bytes_:
        case _zx_tap_hdlss_:
        case _zx_tzx_hdlss_:
            if (block->ini == 0xffff)
                return 2;
            if ((block->ini < 0x4000) && (block->block_type == _zx_bytes_))
                return 1;
            if (block->size + block->ini > MAX_DIR_ZX)
                return 1;
            return 0;
        case _zx_48k_snap_:
        case _zx_128k_snap_:
        case _zx_16k_page_:
        case _zx_complete_:
            return 0;
        case _cpc_basic_:
        case _cpc_ascii_:
        case _cpc_protected_:
            return 2;
        case _cpc_binary_:
            if (block->size + block->ini > 0xff00)
                return 1;
            return 0;
        case _cpc_64k_snap_:
            return 0;
        case _msx_ascii_:
        case _msx_token_:
            return 2;
        case _msx_bytes_:
        case _msx_bin_:
            if (block->size + block->ini > 0xf400)
                return 1;
            return 0;
        case _81_program_:
        case _81_snap_:
            return 0;
        default:
            return 2;
    }
}

void InputDialog::FillData()
{
    m_listView->DeleteAllItems();
    
    m_clearEdt->SetValue(wxT(""));
    m_usrCmbBx->SetValue(wxT(""));
    
    int i = 0;
    while (i < 16)
    {
        if (m_blocksSbb[i].block_type == 0)
            break;
        
        long index = m_listView->InsertItem(m_listView->GetItemCount(), 
                                            wxString(m_blocksSbb[i].blockname, wxConvLocal));
        
        m_listView->SetItemData(index, (wxUIntPtr)&m_blocksSbb[i]);
        
        if (m_blocksSbb[i].block_type != '0')
        {
            wxString sizeStr;
            if (enheaxadecimal)
                sizeStr = wxString::Format(wxT("$%04X"), m_blocksSbb[i].size);
            else
                sizeStr = wxString::Format(wxT("%u"), m_blocksSbb[i].size);
            m_listView->SetItem(index, 1, sizeStr);
            
            m_listView->SetItem(index, 2, wxString(TipoBloque(m_blocksSbb[i].block_type), wxConvLocal));
            
            wxString iniStr;
            if (enheaxadecimal)
                iniStr = wxString::Format(wxT("$%04X"), m_blocksSbb[i].ini);
            else
                iniStr = wxString::Format(wxT("%u"), m_blocksSbb[i].ini);
            m_listView->SetItem(index, 3, iniStr);
            
            wxString execStr;
            if (enheaxadecimal)
                execStr = wxString::Format(wxT("$%04X"), m_blocksSbb[i].exec);
            else
                execStr = wxString::Format(wxT("%u"), m_blocksSbb[i].exec);
            m_listView->SetItem(index, 4, execStr);
            
            m_listView->SetItem(index, 5, wxT(" "));
            m_listView->SetItem(index, 6, wxT(" "));
            m_listView->SetItem(index, 7, wxT(" "));
        }
        else
        {
            buscaUSR(m_blocksSbb[i].data, m_blocksSbb[i].size);
            
            wxString sizeStr;
            if (enheaxadecimal)
                sizeStr = wxString::Format(wxT("$%04X"), m_blocksSbb[i].size);
            else
                sizeStr = wxString::Format(wxT("%u"), m_blocksSbb[i].size);
            m_listView->SetItem(index, 1, sizeStr);
            
            m_listView->SetItem(index, 2, wxString(TipoBloque(m_blocksSbb[i].block_type), wxConvLocal));
            m_listView->SetItem(index, 3, wxT(".."));
            m_listView->SetItem(index, 4, wxT(".."));
            
            wxString param3Str;
            if (enheaxadecimal)
                param3Str = wxString::Format(wxT("$%04X"), m_blocksSbb[i].ini);
            else
                param3Str = wxString::Format(wxT("%u"), m_blocksSbb[i].ini);
            m_listView->SetItem(index, 5, param3Str);
            
            wxString clearStr;
            if (enheaxadecimal)
                clearStr = wxString::Format(wxT("$%04X"), head_sbb.clear_sp);
            else
                clearStr = wxString::Format(wxT("%u"), head_sbb.clear_sp);
            m_listView->SetItem(index, 6, clearStr);
            
            wxString usrStr;
            if (enheaxadecimal)
                usrStr = wxString::Format(wxT("$%04X"), head_sbb.usr_pc);
            else
                usrStr = wxString::Format(wxT("%u"), head_sbb.usr_pc);
            m_listView->SetItem(index, 7, usrStr);
        }
        
        int state = Estate(&m_blocksSbb[i]);
        m_listView->SetItemImage(index, state);
        // Initial check state: Estate returns 0/1 for "auto-include" blocks
        // and >=2 for things that need user confirmation. Mirror that as the
        // initial check, then the user can toggle individual rows.
        m_listView->CheckItem(index, state <= 1);

        i++;
    }
    
    if (head_sbb.clear_sp)
    {
        wxString clearStr;
        if (enheaxadecimal)
            clearStr = wxString::Format(wxT("$%04X"), head_sbb.clear_sp);
        else
            clearStr = wxString::Format(wxT("%u"), head_sbb.clear_sp);
        m_clearEdt->SetValue(clearStr);
    }
    
    if (head_sbb.usr_pc)
    {
        wxString usrStr;
        if (enheaxadecimal)
            usrStr = wxString::Format(wxT("$%04X"), head_sbb.usr_pc);
        else
            usrStr = wxString::Format(wxT("%u"), head_sbb.usr_pc);
        m_usrCmbBx->SetValue(usrStr);
    }
    else if (i > 0)
    {
        head_sbb.usr_pc = m_blocksSbb[i - 1].exec;
        if (head_sbb.usr_pc)
        {
            wxString usrStr;
            if (enheaxadecimal)
                usrStr = wxString::Format(wxT("$%04X"), head_sbb.usr_pc);
            else
                usrStr = wxString::Format(wxT("%u"), head_sbb.usr_pc);
            m_usrCmbBx->SetValue(usrStr);
        }
    }
    
    // Fill memo based on machine type
    m_memo->Clear();
    if (head_sbb.machine[0] == 'Z')
    {
        m_memo->AppendText(wxT("Memory dump of printer buffer:\n"));
        m_memo->AppendText(wxT("------------------------------\n"));
        // TODO: Display printer buffer dump
    }
    else if (head_sbb.machine[0] == 'C')
    {
        m_memo->Hide();
    }
    else if (head_sbb.machine[0] == 'M')
    {
        if (m_blocksSbb[0].block_type == (char)0xEA)
        {
            wxString text((char*)m_blocksSbb[0].data, wxConvLocal);
            m_memo->SetValue(text);
        }
    }
    else if (head_sbb.machine[0] == '8')
    {
        m_memo->AppendText(wxT("Memory dump of PRBUFF+MEMBOT:\n"));
        m_memo->AppendText(wxT("------------------------------\n"));
        // TODO: Display prbuff_membot dump
    }
}

int InputDialog::Abre(const wxString& fichero)
{
    int res = -4;
    wxString extension = wxFileName(fichero).GetExt().Upper();
    
    memset(m_blocksSbb, 0, sizeof(m_blocksSbb));
    strncpy(m_blocksSbb[0].blockname, wxFileName(fichero).GetFullName().mb_str(wxConvLocal), 16);
    m_blocksSbb[0].block_type = 0;
    
    m_memo->Show();
    m_memo->Clear();
    
    wxString mbPath = fichero.mb_str(wxConvLocal);
    
    if (extension == wxT("SBB"))
    {
        m_memo->Hide();
        char mq = head_sbb.machine[0];
        res = load_sbb_file(mbPath.c_str(), &head_sbb, m_blocksSbb);
        if (!res && mq != head_sbb.machine[0])
            res = -6;
    }
    else if (extension == wxT("HST"))
    {
        m_memo->Hide();
        res = load_hst_file(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("SCR"))
    {
        m_memo->Hide();
        res = leescr(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("CAS"))
    {
        res = leecas(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("CDT") || extension == wxT("TZX"))
    {
        res = leetzx(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("TAP"))
    {
        res = leetap(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("Z80"))
    {
        res = leez80(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("SNA"))
    {
        if (head_sbb.machine[0] == 'C')
            res = lee_cpc_sna(mbPath.c_str(), &head_sbb, m_blocksSbb);
        else
            res = lee_zx_sna(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("BIN"))
    {
        if (head_sbb.machine[0] == 'C')
            res = lee_cpc_bin(mbPath.c_str(), &head_sbb, m_blocksSbb);
        else
            res = lee_msx_bin(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("ROM"))
    {
        res = ROMMSX(mbPath.c_str());
    }
    else if (extension == wxT("P"))
    {
        res = lee_p(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("Z81"))
    {
        res = lee_z81(mbPath.c_str(), &head_sbb, m_blocksSbb);
    }
    else if (extension == wxT("DSK"))
    {
        wxString command;
        if (head_sbb.machine[0] == 'C')
            command = ToolCPC + wxT(" \"") + fichero + wxT("\"");
        else
            command = ToolDiskMgr + wxT(" \"") + fichero + wxT("\"");
        wxExecute(command, wxEXEC_SYNC);
        wxMessageBox(wxString(error_file(res), wxConvLocal), wxT("Error"), wxOK | wxICON_ERROR, this);
        return wxID_CANCEL;
    }
    
    if (res == 0)
    {
        FillData();
        return ShowModal();
    }
    else
    {
        wxString msg = fichero + wxT("\r\n");
        msg += wxString(error_file(res), wxConvLocal);
        wxMessageBox(msg, wxT("Error"), wxOK | wxICON_ERROR, this);
        return wxID_CANCEL;
    }
}

int InputDialog::Edit(ts_sbb_header* in_head_sbb, ts_sbb_block* in_blocks_sbb)
{
    head_sbb = *in_head_sbb;
    for (int i = 0; i < 15; i++)
        m_blocksSbb[i] = in_blocks_sbb[i];
    FillData();
    return ShowModal();
}

int InputDialog::ROMMSX(const char* file)
{
    return lee_msx_rom(file, &head_sbb, m_blocksSbb, nullptr);
}

void InputDialog::OnOK(wxCommandEvent& event)
{
    int b = 0;
    
    wxString clearStr = m_clearEdt->GetValue();
    head_sbb.clear_sp = inputDecHex(clearStr.mb_str(wxConvLocal));
    
    wxString usrStr = m_usrCmbBx->GetValue();
    head_sbb.usr_pc = inputDecHex(usrStr.mb_str(wxConvLocal));
    
    if (!res_blocks_sbb)
        return;
    
    for (int i = 0; i < m_listView->GetItemCount(); i++)
    {
        // Pick up the user's explicit checkbox choice (replaces the previous
        // implicit Estate() <= 1 filter, which couldn't be overridden because
        // there was no visible UI for it).
        if (!m_listView->IsItemChecked(i))
            continue;

        res_blocks_sbb[b] = m_blocksSbb[i];

        if ((head_sbb.machine[0] == 'Z') && m_blocksSbb[i].block_type == _zx_bytes_)
        {
            if ((m_blocksSbb[i].ini + m_blocksSbb[i].size - 1) >= MAX_DIR_ZX)
            {
                zxlastbytes(&head_sbb, &res_blocks_sbb[b]);
                b++;
            }
        }
        b++;
    }
    res_blocks_sbb[b].block_type = 0;
    
    EndModal(wxID_OK);
}

void InputDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void InputDialog::OnModify(wxCommandEvent& event)
{
    long item = m_listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item == -1 || item >= 16)
        return;
    
    // Use item index directly to access block
    if (m_blocksSbb[item].block_type == 0)
        return;
    
    ModifyDialog dlg(this);
    dlg.SetBlock(&m_blocksSbb[item]);
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.GetBlock(&m_blocksSbb[item]);
        FillData();
    }
}

void InputDialog::OnSeeData(wxCommandEvent& event)
{
    long item = m_listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item == -1 || item >= 16)
        return;
    
    // Use item index directly to access block
    if (m_blocksSbb[item].block_type == 0)
        return;
    
    ModifyDialog dlg(this);
    dlg.SetBlock(&m_blocksSbb[item]);
    dlg.ShowModal();
}

void InputDialog::OnConvertBasic(wxCommandEvent& event)
{
    long item = m_listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item == -1 || item >= 16)
        return;
    
    // Use item index directly to access block
    if (m_blocksSbb[item].block_type == 0)
        return;
    
    ts_sbb_block back = m_blocksSbb[item];
    zxbasic2bytes(&m_blocksSbb[item]);
    OnModify(event);
    
    wxString usrStr;
    if (enheaxadecimal)
        usrStr = wxString::Format(wxT("$%04X"), m_blocksSbb[item].exec);
    else
        usrStr = wxString::Format(wxT("%u"), m_blocksSbb[item].exec);
    m_usrCmbBx->SetValue(usrStr);
}

void InputDialog::OnRestore(wxCommandEvent& event)
{
    long item = m_listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item == -1 || item >= 16)
        return;
    
    // Use item index directly to access block
    if (m_blocksSbb[item].block_type == 0)
        return;
    
    ts_sbb_block back = m_blocksSbb[item];
    restorefromscreen(&m_blocksSbb[item]);
    OnModify(event);
}

void InputDialog::OnViewHex(wxCommandEvent& event)
{
    enheaxadecimal = !enheaxadecimal;
    FillData();
}

void InputDialog::OnPopupMenu(wxContextMenuEvent& event)
{
    // "View decimal/hexadecimal" is always enabled
    m_popupMenu->Enable(m_menuIdViewHex, true);
    
    // Default: disable all other menu items
    m_popupMenu->Enable(m_menuIdModify, false);
    m_popupMenu->Enable(m_menuIdSeeData, false);
    m_popupMenu->Enable(m_menuIdConvertBasic, false);
    m_popupMenu->Enable(m_menuIdRestore, false);
    
    // Get the item at the click position
    int flags = 0;
    long item = m_listView->HitTest(event.GetPosition(), flags);
    
    // If no item found at click position, try selected item
    if (item == wxNOT_FOUND)
    {
        item = m_listView->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }
    
    // If still no item, try to select the item at the click position
    if (item == wxNOT_FOUND)
    {
        int flags2 = 0;
        item = m_listView->HitTest(event.GetPosition(), flags2);
        if (item != wxNOT_FOUND && item >= 0 && item < m_listView->GetItemCount())
        {
            m_listView->SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        }
    }
    
    if (item != wxNOT_FOUND && item >= 0 && item < m_listView->GetItemCount())
    {
        // Use item index directly instead of relying on pointer validation
        // since ItemData may point to external blocks
        if (item < 16 && m_blocksSbb[item].block_type != 0)
        {
            ts_sbb_block* safeBlock = &m_blocksSbb[item];
            m_popupMenu->Enable(m_menuIdModify, true);
            m_popupMenu->Enable(m_menuIdSeeData, true);
            m_popupMenu->Enable(m_menuIdConvertBasic, safeBlock->block_type == _zx_program_);
            m_popupMenu->Enable(m_menuIdRestore, 
                                (safeBlock->block_type == _zx_48k_snap_) && 
                                (safeBlock->data[0x1b00 + 40] != 0xc3) && 
                                m_memo->IsShown());
        }
    }
    PopupMenu(m_popupMenu);
}

void InputDialog::OnListItemSelected(wxListEvent& event)
{
    // Update UI based on selection
}

