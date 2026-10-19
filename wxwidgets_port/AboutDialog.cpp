#include "AboutDialog.h"

#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/font.h>
#include <wx/panel.h>
#include <wx/settings.h>

#include "../source/sbb.h"  // version_SBB

#ifdef __has_include
    #if __has_include("embedded_bmp_resources.h")
        #include "embedded_bmp_resources.h"
        #define HAVE_EMBEDDED_OTLA_LOGO 1
    #endif
#endif
#ifndef HAVE_EMBEDDED_OTLA_LOGO
    #define HAVE_EMBEDDED_OTLA_LOGO 0
#endif

AboutDialog::AboutDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("About"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE)
{
    // Plain panel with a simple border (no 3D bevel), matching the original
    wxPanel* panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxBORDER_SIMPLE);

    // Logo on the left
    wxBitmap logoBmp;
#if HAVE_EMBEDDED_OTLA_LOGO
    wxImage logoImg = LoadOtla_logoBmp();
    if (logoImg.IsOk())
        logoBmp = wxBitmap(logoImg);
#endif

    wxStaticBitmap* logo = nullptr;
    if (logoBmp.IsOk())
        logo = new wxStaticBitmap(panel, wxID_ANY, logoBmp);

    // Right column: product, version, developer, email
    wxString productText = wxT("Proyecto O.T.L.A.");
    wxString versionText = wxString::Format(wxT("Version 2.2.5"));
    wxString versionTextSBB = wxString::Format(wxT("SBB Version %s"),
                                            wxString(version_SBB, wxConvLocal).c_str());
    wxString developedByText = wxT("Developed by");
    // Use \u-escape so this works whether MSVC reads the .cpp as UTF-8 or as
    // the system codepage. wxT("...") expands to L"..." (wide string literal)
    // on Unicode wxWidgets builds, where í is correctly U+00ED ("i acute").
    wxString authorText = wxT("Francisco Villa Rodr\u00EDguez");
    wxString emailText = wxT("proyectootla@gmail.com");

    wxStaticText* productLbl = new wxStaticText(panel, wxID_ANY, productText);
    wxFont boldFont = productLbl->GetFont();
    boldFont.SetWeight(wxFONTWEIGHT_BOLD);
    productLbl->SetFont(boldFont);

    wxStaticText *version = new wxStaticText(panel, wxID_ANY, versionText);
    wxStaticText *versionLbl = new wxStaticText(panel, wxID_ANY, versionTextSBB);
    wxStaticText* developedByLbl = new wxStaticText(panel, wxID_ANY, developedByText);
    wxStaticText* authorLbl = new wxStaticText(panel, wxID_ANY, authorText);
    wxStaticText* emailLbl = new wxStaticText(panel, wxID_ANY, emailText);

    wxString portText = wxT("Improvements and wxWidgets Port by\nPossi");
    wxStaticText* portLbl = new wxStaticText(panel, wxID_ANY, portText);
    wxFont italicFont = portLbl->GetFont();
    italicFont.SetStyle(wxFONTSTYLE_ITALIC);
    portLbl->SetFont(italicFont);

    wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);
    textSizer->Add(productLbl, 0, wxBOTTOM, 4);
    textSizer->Add(version, 0, wxBOTTOM, 2);
    textSizer->Add(versionLbl, 0, wxBOTTOM, 12);
    textSizer->Add(developedByLbl, 0, wxBOTTOM, 2);
    textSizer->Add(authorLbl, 0, wxBOTTOM, 4);
    textSizer->Add(emailLbl, 0, wxBOTTOM, 12);
    textSizer->Add(portLbl, 0);

    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
    if (logo)
        topSizer->Add(logo, 0, wxALL, 12);
    else
        topSizer->AddSpacer(12);
    topSizer->Add(textSizer, 1, wxALIGN_CENTER_VERTICAL | wxALL, 12);

    panel->SetSizerAndFit(topSizer);

    // OK button below the panel
    wxButton* okBtn = new wxButton(this, wxID_OK, wxT("OK"));
    okBtn->SetDefault();

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(panel, 1, wxEXPAND | wxALL, 8);
    mainSizer->Add(okBtn, 0, wxALIGN_CENTER | wxBOTTOM, 8);

    SetSizerAndFit(mainSizer);
    CentreOnParent();
}

AboutDialog::~AboutDialog()
{
}
