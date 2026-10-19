#include <wx/wx.h>
#include "MainFrame.h"

class OtlaApp : public wxApp
{
public:
    virtual bool OnInit() override;
    
    // Store command line arguments for later use
    wxString m_cmdLineFile;
};

wxIMPLEMENT_APP(OtlaApp);

bool OtlaApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    // Check for command line file argument
    if (argc > 1)
    {
        m_cmdLineFile = wxString(argv[1]);
    }

    MainFrame *frame = new MainFrame();
    
    // Pass command line file if provided
    if (!m_cmdLineFile.IsEmpty())
    {
        frame->OpenFileSBB(m_cmdLineFile);
    }
    
    frame->Show(true);
    
    return true;
}


