#include "WavMonitorDialog.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/fileconf.h>
#include <wx/stdpaths.h>
#include <wx/dcclient.h>
#include <cstring>
#include <cstdio>

// ---------------------------------------------------------------------------
// miniaudio C callback bridge
// ---------------------------------------------------------------------------

static void ma_data_callback_impl(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount)
{
    WavMonitorDialog* self = static_cast<WavMonitorDialog*>(pDevice->pUserData);
    if (self)
        self->FillAudioBuffer(pOutput, frameCount);
}

void WavMonitorDialog::FillAudioBuffer(void* output, unsigned int frameCount)
{
    ma_decoder* dec = static_cast<ma_decoder*>(m_decoder);
    if (!dec)
    {
        std::memset(output, 0, (size_t)frameCount * m_channels * sizeof(short));
        return;
    }

    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(dec, output, frameCount, &framesRead);

    if (framesRead < frameCount)
    {
        // Zero-fill the tail so we don't replay stale buffer contents at EOF
        size_t bytesPerFrame = m_channels * sizeof(short);
        std::memset(static_cast<char*>(output) + framesRead * bytesPerFrame,
                    0,
                    (frameCount - framesRead) * bytesPerFrame);
    }
}

// ---------------------------------------------------------------------------
// Event table
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(WavMonitorDialog, wxDialog)
    EVT_TIMER(wxID_ANY, WavMonitorDialog::OnTimer)
    EVT_CLOSE(WavMonitorDialog::OnCloseWindow)
wxEND_EVENT_TABLE()

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WavMonitorDialog::WavMonitorDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("WAV Monitor"), wxDefaultPosition, wxSize(600, 400)),
      m_playBtn(nullptr),
      m_stopBtn(nullptr),
      m_openBtn(nullptr),
      m_deviceCmbBx(nullptr),
      m_volumeSlider(nullptr),
      m_muteChkBx(nullptr),
      m_closeOnChkBx(nullptr),
      m_progressBar(nullptr),
      m_fileLbl(nullptr),
      m_timeLbl(nullptr),
      m_formatLbl(nullptr),
      m_wavePanel(nullptr),
      m_timer(nullptr),
      m_context(nullptr),
      m_decoder(nullptr),
      m_device(nullptr),
      m_totalFrames(0),
      m_cursorFrames(0),
      m_sampleRate(44100),
      m_channels(1),
      m_playing(false),
      m_paused(false),
      m_audioDevice(0)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // File info
    wxBoxSizer* fileSizer = new wxBoxSizer(wxHORIZONTAL);
    m_fileLbl = new wxStaticText(this, wxID_ANY, wxT("No file loaded"));
    fileSizer->Add(m_fileLbl, 1, wxEXPAND | wxALL, 5);
    m_openBtn = new wxButton(this, wxID_ANY, wxT("Open..."));
    m_openBtn->Bind(wxEVT_BUTTON, &WavMonitorDialog::OnOpen, this);
    fileSizer->Add(m_openBtn, 0, wxALL, 5);
    mainSizer->Add(fileSizer, 0, wxEXPAND);

    // Format info
    m_formatLbl = new wxStaticText(this, wxID_ANY, wxT(""));
    mainSizer->Add(m_formatLbl, 0, wxALL, 5);

    // Device selection
    wxBoxSizer* deviceSizer = new wxBoxSizer(wxHORIZONTAL);
    deviceSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Device:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_deviceCmbBx = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                    0, nullptr, wxCB_READONLY);
    m_deviceCmbBx->Bind(wxEVT_COMBOBOX, &WavMonitorDialog::OnDeviceChange, this);
    deviceSizer->Add(m_deviceCmbBx, 1, wxEXPAND | wxRIGHT, 5);
    mainSizer->Add(deviceSizer, 0, wxEXPAND | wxALL, 5);

    // Volume control
    wxBoxSizer* volumeSizer = new wxBoxSizer(wxHORIZONTAL);
    volumeSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Volume:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_volumeSlider = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(200, -1));
    m_volumeSlider->Bind(wxEVT_SLIDER, &WavMonitorDialog::OnVolumeChange, this);
    volumeSizer->Add(m_volumeSlider, 1, wxEXPAND | wxRIGHT, 5);
    m_muteChkBx = new wxCheckBox(this, wxID_ANY, wxT("Mute"));
    m_muteChkBx->Bind(wxEVT_CHECKBOX, &WavMonitorDialog::OnMute, this);
    volumeSizer->Add(m_muteChkBx, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(volumeSizer, 0, wxEXPAND | wxALL, 5);

    // Progress bar
    m_progressBar = new wxGauge(this, wxID_ANY, 100);
    mainSizer->Add(m_progressBar, 0, wxEXPAND | wxALL, 5);

    // Time label
    m_timeLbl = new wxStaticText(this, wxID_ANY, wxT("Time: 0.0"));
    mainSizer->Add(m_timeLbl, 0, wxALL, 5);

    // Waveform display
    m_wavePanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 100));
    m_wavePanel->SetBackgroundColour(wxColour(0, 0, 0));
    m_wavePanel->Bind(wxEVT_PAINT, &WavMonitorDialog::OnPaint, this);
    mainSizer->Add(m_wavePanel, 1, wxEXPAND | wxALL, 5);

    // Buttons
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_playBtn = new wxButton(this, wxID_ANY, wxT("Play"));
    m_playBtn->Bind(wxEVT_BUTTON, &WavMonitorDialog::OnPlay, this);
    m_stopBtn = new wxButton(this, wxID_ANY, wxT("Stop"));
    m_stopBtn->Bind(wxEVT_BUTTON, &WavMonitorDialog::OnStop, this);
    m_closeOnChkBx = new wxCheckBox(this, wxID_ANY, wxT("Close when finished"));
    m_closeOnChkBx->SetValue(true);
    btnSizer->Add(m_playBtn, 0, wxRIGHT, 5);
    btnSizer->Add(m_stopBtn, 0, wxRIGHT, 5);
    btnSizer->Add(m_closeOnChkBx, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    btnSizer->AddStretchSpacer();
    wxButton* closeBtn = new wxButton(this, wxID_CLOSE, wxT("Close"));
    closeBtn->Bind(wxEVT_BUTTON, &WavMonitorDialog::OnClose, this);
    btnSizer->Add(closeBtn, 0);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);

    m_playBtn->Enable(false);
    m_stopBtn->Enable(false);

    m_timer = new wxTimer(this);

    SetSizer(mainSizer);
    Centre();

    // Init miniaudio context and enumerate devices
    ma_context* ctx = new ma_context;
    if (ma_context_init(NULL, 0, NULL, ctx) == MA_SUCCESS)
    {
        m_context = ctx;
        EnumerateDevices();
    }
    else
    {
        delete ctx;
        m_deviceCmbBx->Append(wxT("Default"));
        m_deviceIds.push_back(nullptr);
    }

    LoadSettings();
}

WavMonitorDialog::~WavMonitorDialog()
{
    SaveSettings();
    StopPlayback();
    ReleaseAudio();

    for (void* id : m_deviceIds)
        delete static_cast<ma_device_id*>(id);
    m_deviceIds.clear();

    if (m_context)
    {
        ma_context* ctx = static_cast<ma_context*>(m_context);
        ma_context_uninit(ctx);
        delete ctx;
        m_context = nullptr;
    }

    if (m_timer)
    {
        delete m_timer;
        m_timer = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Device enumeration
// ---------------------------------------------------------------------------

void WavMonitorDialog::EnumerateDevices()
{
    m_deviceCmbBx->Clear();
    for (void* id : m_deviceIds)
        delete static_cast<ma_device_id*>(id);
    m_deviceIds.clear();

    // Index 0 is always "System default" (NULL device id)
    m_deviceCmbBx->Append(wxT("System default"));
    m_deviceIds.push_back(nullptr);

    if (!m_context)
        return;

    ma_context* ctx = static_cast<ma_context*>(m_context);
    ma_device_info* pPlaybackInfos = nullptr;
    ma_uint32 playbackCount = 0;
    if (ma_context_get_devices(ctx, &pPlaybackInfos, &playbackCount, NULL, NULL) != MA_SUCCESS)
        return;

    for (ma_uint32 i = 0; i < playbackCount; i++)
    {
        m_deviceCmbBx->Append(wxString::FromUTF8(pPlaybackInfos[i].name));
        ma_device_id* idCopy = new ma_device_id(pPlaybackInfos[i].id);
        m_deviceIds.push_back(idCopy);
    }

    m_deviceCmbBx->SetSelection(0);
}

// ---------------------------------------------------------------------------
// Settings (per-user otla.ini)
// ---------------------------------------------------------------------------

void WavMonitorDialog::LoadSettings()
{
    wxString iniPath = wxStandardPaths::Get().GetUserConfigDir() + wxFILE_SEP_PATH + wxT("otla.ini");
    wxFileConfig config(wxT("OTLA"), wxEmptyString, iniPath);

    m_audioDevice = (int)config.ReadLong(wxT("/otlaPlayer/Audiodevice"), 0L);
    m_closeOnChkBx->SetValue(config.ReadBool(wxT("/otlaPlayer/CloseOnFinished"), true));

    if (m_audioDevice >= 0 && m_audioDevice < (int)m_deviceCmbBx->GetCount())
        m_deviceCmbBx->SetSelection(m_audioDevice);
    else
        m_deviceCmbBx->SetSelection(0);
}

void WavMonitorDialog::SaveSettings()
{
    wxString iniPath = wxStandardPaths::Get().GetUserConfigDir() + wxFILE_SEP_PATH + wxT("otla.ini");
    wxFileConfig config(wxT("OTLA"), wxEmptyString, iniPath);

    config.Write(wxT("/otlaPlayer/Audiodevice"), (long)m_deviceCmbBx->GetSelection());
    config.Write(wxT("/otlaPlayer/CloseOnFinished"), m_closeOnChkBx->GetValue());
    config.Flush();
}

// ---------------------------------------------------------------------------
// File loading
// ---------------------------------------------------------------------------

int WavMonitorDialog::OpenWaveFile(const wxString& filename)
{
    StopPlayback();
    ReleaseAudio();

    // Configure decoder to deliver signed 16-bit PCM (used by both playback and the visualizer)
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_s16, 0, 0);

    ma_decoder* dec = new ma_decoder;
    wxScopedCharBuffer utf8 = filename.utf8_str();
    if (ma_decoder_init_file(utf8.data(), &decoderConfig, dec) != MA_SUCCESS)
    {
        delete dec;
        wxMessageBox(wxT("Error opening WAV file"), wxT("Error"), wxOK | wxICON_ERROR);
        return -1;
    }

    m_decoder = dec;
    m_waveFile = filename;
    m_fileLbl->SetLabel(wxFileName(filename).GetFullName());

    m_sampleRate = dec->outputSampleRate;
    m_channels = dec->outputChannels;

    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(dec, &totalFrames);
    m_totalFrames = totalFrames;
    m_cursorFrames = 0;

    // Decode the whole file into memory once for waveform visualization
    m_pcmFrames.clear();
    if (m_totalFrames > 0)
    {
        m_pcmFrames.resize((size_t)(m_totalFrames * m_channels));
        ma_uint64 framesRead = 0;
        ma_decoder_read_pcm_frames(dec, m_pcmFrames.data(), m_totalFrames, &framesRead);
        if (framesRead < m_totalFrames)
            m_pcmFrames.resize((size_t)(framesRead * m_channels));
        m_totalFrames = framesRead;
        ma_decoder_seek_to_pcm_frame(dec, 0);
    }

    m_progressBar->SetRange((int)(m_totalFrames > 0x7fffffff ? 0x7fffffff : m_totalFrames));
    m_progressBar->SetValue(0);

    m_formatLbl->SetLabel(wxString::Format(wxT("%u Hz %u bit %s"),
                                           m_sampleRate,
                                           16,
                                           m_channels == 1 ? wxT("mono") : wxT("stereo")));

    m_playBtn->Enable(true);
    m_stopBtn->Enable(false);

    if (m_wavePanel)
        m_wavePanel->Refresh(false);
    return 0;
}

// ---------------------------------------------------------------------------
// Playback control
// ---------------------------------------------------------------------------

bool WavMonitorDialog::StartAudioDevice()
{
    if (!m_decoder)
        return false;

    ma_decoder* dec = static_cast<ma_decoder*>(m_decoder);

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_s16;
    config.playback.channels = m_channels;
    config.sampleRate = m_sampleRate;
    config.dataCallback = ma_data_callback_impl;
    config.pUserData = this;

    int sel = m_deviceCmbBx->GetSelection();
    if (sel > 0 && sel < (int)m_deviceIds.size() && m_deviceIds[sel])
        config.playback.pDeviceID = static_cast<ma_device_id*>(m_deviceIds[sel]);

    ma_device* dev = new ma_device;
    if (ma_device_init(static_cast<ma_context*>(m_context), &config, dev) != MA_SUCCESS)
    {
        delete dev;
        wxMessageBox(wxT("Failed to initialize audio device"), wxT("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    m_device = dev;
    ApplyVolume();

    if (ma_device_start(dev) != MA_SUCCESS)
    {
        ma_device_uninit(dev);
        delete dev;
        m_device = nullptr;
        wxMessageBox(wxT("Failed to start audio device"), wxT("Error"), wxOK | wxICON_ERROR);
        return false;
    }

    (void)dec;
    return true;
}

void WavMonitorDialog::ReleaseAudio()
{
    if (m_device)
    {
        ma_device* dev = static_cast<ma_device*>(m_device);
        ma_device_uninit(dev);
        delete dev;
        m_device = nullptr;
    }
    if (m_decoder)
    {
        ma_decoder* dec = static_cast<ma_decoder*>(m_decoder);
        ma_decoder_uninit(dec);
        delete dec;
        m_decoder = nullptr;
    }
}

void WavMonitorDialog::Play()
{
    wxCommandEvent dummy;
    OnPlay(dummy);
}

void WavMonitorDialog::OnPlay(wxCommandEvent& /*event*/)
{
    if (!m_decoder)
        return;

    if (m_playing)
    {
        // Toggle pause
        m_paused = !m_paused;
        if (m_device)
        {
            ma_device* dev = static_cast<ma_device*>(m_device);
            if (m_paused)
                ma_device_stop(dev);
            else
                ma_device_start(dev);
        }
        m_playBtn->SetLabel(m_paused ? wxT("Play") : wxT("Pause"));
    }
    else
    {
        if (!StartAudioDevice())
            return;
        m_playing = true;
        m_paused = false;
        m_playBtn->SetLabel(wxT("Pause"));
        m_stopBtn->Enable(true);
        m_deviceCmbBx->Enable(false);
        m_openBtn->Enable(false);
        m_timer->Start(50);  // UI refresh ~20 Hz
    }
}

void WavMonitorDialog::OnStop(wxCommandEvent& /*event*/)
{
    StopPlayback();
}

void WavMonitorDialog::StopPlayback()
{
    if (m_timer && m_timer->IsRunning())
        m_timer->Stop();

    if (m_device)
    {
        ma_device* dev = static_cast<ma_device*>(m_device);
        ma_device_uninit(dev);
        delete dev;
        m_device = nullptr;
    }

    if (m_decoder)
    {
        ma_decoder* dec = static_cast<ma_decoder*>(m_decoder);
        ma_decoder_seek_to_pcm_frame(dec, 0);
    }

    m_playing = false;
    m_paused = false;
    m_cursorFrames = 0;

    m_playBtn->SetLabel(wxT("Play"));
    m_playBtn->Enable(m_decoder != nullptr);
    m_stopBtn->Enable(false);
    m_deviceCmbBx->Enable(true);
    m_openBtn->Enable(true);
    m_progressBar->SetValue(0);
    m_timeLbl->SetLabel(wxT("Time: 0.0"));

    if (m_wavePanel)
        m_wavePanel->Refresh(false);
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void WavMonitorDialog::OnOpen(wxCommandEvent& /*event*/)
{
    wxFileDialog dlg(this, wxT("Open WAV File"), wxT(""), wxT(""),
                     wxT("WAV files (*.wav)|*.wav|All files (*.*)|*.*"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dlg.ShowModal() == wxID_OK)
        OpenWaveFile(dlg.GetPath());
}

void WavMonitorDialog::OnClose(wxCommandEvent& /*event*/)
{
    StopPlayback();
    EndModal(wxID_CLOSE);
}

void WavMonitorDialog::OnTimer(wxTimerEvent& /*event*/)
{
    if (!m_playing || m_paused || !m_decoder)
        return;

    ma_decoder* dec = static_cast<ma_decoder*>(m_decoder);
    ma_uint64 cursor = 0;
    ma_decoder_get_cursor_in_pcm_frames(dec, &cursor);
    m_cursorFrames = cursor;

    UpdatePosition();

    if (m_totalFrames > 0 && m_cursorFrames >= m_totalFrames)
    {
        StopPlayback();
        if (m_closeOnChkBx->IsChecked())
            EndModal(wxID_CLOSE);
        return;
    }

    if (m_wavePanel)
        m_wavePanel->Refresh(false);
}

void WavMonitorDialog::UpdatePosition()
{
    if (m_sampleRate == 0)
        return;

    int progressMax = m_progressBar->GetRange();
    int value = (int)m_cursorFrames;
    if (value > progressMax)
        value = progressMax;
    m_progressBar->SetValue(value);

    float seconds = (float)m_cursorFrames / (float)m_sampleRate;
    m_timeLbl->SetLabel(wxString::Format(wxT("Time: %.1f"), seconds));
}

void WavMonitorDialog::ApplyVolume()
{
    if (!m_device)
        return;
    ma_device* dev = static_cast<ma_device*>(m_device);
    float vol = m_muteChkBx->GetValue() ? 0.0f : (m_volumeSlider->GetValue() / 100.0f);
    ma_device_set_master_volume(dev, vol);
}

void WavMonitorDialog::OnDeviceChange(wxCommandEvent& /*event*/)
{
    m_audioDevice = m_deviceCmbBx->GetSelection();
    // Device change while playing: stop, swap, user can press Play again.
    if (m_playing)
        StopPlayback();
}

void WavMonitorDialog::OnVolumeChange(wxCommandEvent& /*event*/)
{
    ApplyVolume();
}

void WavMonitorDialog::OnMute(wxCommandEvent& /*event*/)
{
    ApplyVolume();
}

void WavMonitorDialog::OnCloseWindow(wxCloseEvent& event)
{
    StopPlayback();
    event.Skip();
}

// ---------------------------------------------------------------------------
// Waveform visualization
// ---------------------------------------------------------------------------

void WavMonitorDialog::OnPaint(wxPaintEvent& /*event*/)
{
    wxPaintDC dc(m_wavePanel);
    DrawWaveform(dc);
}

void WavMonitorDialog::DrawWaveform(wxDC& dc)
{
    wxSize size = m_wavePanel->GetClientSize();
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxBLACK_BRUSH);
    dc.DrawRectangle(0, 0, size.x, size.y);

    if (m_pcmFrames.empty() || m_totalFrames == 0 || m_cursorFrames >= m_totalFrames)
        return;

    int centerY = size.y / 2;
    int samplesAcross = size.x / 3;
    if (samplesAcross <= 1)
        return;

    // Match original behavior: one sample per column starting at the cursor,
    // ~size.x/3 frames total (about 4-5 ms at 44.1 kHz). Connect samples with
    // continuous line segments rather than isolated dashes.
    int count = samplesAcross;
    if (m_cursorFrames + (ma_uint64)count > m_totalFrames)
        count = (int)(m_totalFrames - m_cursorFrames);
    if (count <= 1)
        return;

    std::vector<wxPoint> points;
    points.reserve(count);
    for (int x = 0; x < count; x++)
    {
        short sample = m_pcmFrames[(size_t)((m_cursorFrames + x) * m_channels)];
        int y = centerY - (sample * (centerY - 2)) / 32768;
        points.emplace_back(x * 3, y);
    }

    dc.SetPen(wxPen(*wxGREEN, 2));
    dc.DrawLines((int)points.size(), points.data());
}
