#ifndef WAVMONITORDIALOG_H
#define WAVMONITORDIALOG_H

#include <wx/dialog.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/filedlg.h>
#include <wx/panel.h>
#include <wx/dcclient.h>
#include <wx/fileconf.h>
#include <vector>

class WavMonitorDialog : public wxDialog
{
public:
    WavMonitorDialog(wxWindow* parent);
    ~WavMonitorDialog();

    int OpenWaveFile(const wxString& filename);
    void Play();

    // Called from the miniaudio data callback (audio thread).
    // Public so the static C callback can reach it.
    void FillAudioBuffer(void* output, unsigned int frameCount);

private:
    void OnOpen(wxCommandEvent& event);
    void OnPlay(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnDeviceChange(wxCommandEvent& event);
    void OnVolumeChange(wxCommandEvent& event);
    void OnMute(wxCommandEvent& event);
    void OnCloseWindow(wxCloseEvent& event);
    void OnPaint(wxPaintEvent& event);

    void UpdatePosition();
    void ApplyVolume();
    void DrawWaveform(wxDC& dc);
    void StopPlayback();
    void EnumerateDevices();
    void ReleaseAudio();
    bool StartAudioDevice();
    void LoadSettings();
    void SaveSettings();

    wxButton* m_playBtn;
    wxButton* m_stopBtn;
    wxButton* m_openBtn;
    wxComboBox* m_deviceCmbBx;
    wxSlider* m_volumeSlider;
    wxCheckBox* m_muteChkBx;
    wxCheckBox* m_closeOnChkBx;
    wxGauge* m_progressBar;
    wxStaticText* m_fileLbl;
    wxStaticText* m_timeLbl;
    wxStaticText* m_formatLbl;
    wxPanel* m_wavePanel;

    wxTimer* m_timer;
    wxString m_waveFile;

    // Opaque miniaudio handles (allocated in .cpp to keep miniaudio.h out of this header)
    void* m_context;   // ma_context*
    void* m_decoder;   // ma_decoder*
    void* m_device;    // ma_device*

    // Device IDs cached during enumeration (one entry per combo box item; index 0 == default/NULL)
    std::vector<void*> m_deviceIds;  // entries are heap-allocated copies of ma_device_id

    // Decoded sample buffer for waveform drawing (signed 16-bit PCM, interleaved)
    std::vector<short> m_pcmFrames;
    unsigned long long m_totalFrames;
    unsigned long long m_cursorFrames;   // last known playback cursor (updated by timer)
    unsigned int m_sampleRate;
    unsigned int m_channels;

    bool m_playing;
    bool m_paused;
    int m_audioDevice;

    wxDECLARE_EVENT_TABLE();
};

#endif
