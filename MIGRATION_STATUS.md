# wxWidgets Port — Migration Status

## Framework

Ported from **Borland C++ Builder 6 / VCL** to **wxWidgets 3.3.1** using **CMake + MSVC (Visual Studio 2022)** and **vcpkg** for dependency management.

---

## Completed

### Build System
- CMakeLists.txt with vcpkg toolchain support
- Python-based BMP logo embedding (`embed_bmp.py`) — generates `embedded_bmp_resources.h/cpp` at build time
- Windows icon (`otla.ico`) copied to build output
- VS Code tasks and launch configuration

### Application Entry Point
| Before (Borland) | After (wxWidgets) |
|-----------------|-------------------|
| `WinMain` + `Application->CreateForm(__classid(...))` | `wxApp::OnInit()` + `wxIMPLEMENT_APP` |

### Core Logic Files (all linked unchanged)
- `funciones.cpp/h`, `sbb.cpp/h`, `wav2.cpp/h`, `loaders.cpp/h`
- `zxfiles.cpp/h`, `81files.cpp/h`, `cpcfiles.cpp/h`, `msxfiles.cpp/h`, `signaltest.cpp/h`
- Fix applied: `#include <mem.h>` → `#include <string.h>` in `sbb.cpp`, `wav2.cpp`, `signaltest.cpp`

### GUI — All Forms Ported (code-based, no `.dfm`)

| Original form | wxWidgets file | Notes |
|--------------|----------------|-------|
| `mainForm` | `MainFrame.cpp/h` | Main window, menus, file ops, settings |
| `AboutForm` | `AboutDialog.cpp/h` | Minimal version |
| `SettingsForm` + `OptionsForm` + `ToolsForm` | `SettingsDialog.cpp/h` | Merged (OptionsForm/ToolsForm were unused in original) |
| `DirForm` | `DirDialog.cpp/h` | Directory selection |
| `ModificaForm` | `ModifyDialog.cpp/h` | Block modification |
| `InputForm` | `InputDialog.cpp/h` | File loading, MSX ROM handling |
| `BatchForm` | `BatchDialog.cpp/h` | Batch processing |
| `AdjustForm` | `AdjustDialog.cpp/h` | Audio adjustment, signal test |
| `wavmonitorForm` | `WavMonitorDialog.cpp/h` | WAV UI + file reader + waveform display |

### API Replacements

| Borland / Windows API | wxWidgets equivalent |
|-----------------------|----------------------|
| `AnsiString` | `wxString` |
| `TIniFile` | `wxFileConfig` |
| `__fastcall` event handlers | Standard member functions |
| `#pragma hdrstop/package/resource` | Removed |
| `Application->ExeName` | `wxStandardPaths::Get().GetExecutablePath()` |
| `ExtractFilePath/ExtractFilename` | `wxFileName` |
| `TOpenDialog / TSaveDialog` | `wxFileDialog` |
| `TListView` | `wxListCtrl` |
| `TTrackBar` | `wxSlider` |
| `TProgressBar` | `wxGauge` |
| `TTimer` | `wxTimer` |
| `TBitmapComboBox` | `wxBitmapComboBox` |
| `Canvas->LineTo/MoveTo` (waveform) | `wxDC::DrawLine` |
| `MMIO` WAV file reading | `ma_decoder_init_file` (miniaudio) |
| `waveOut*` playback API | `ma_device` (miniaudio, WASAPI on Windows) |
| `mixer*` volume API | `ma_device_set_master_volume` |
| `MessageBox` / `Application->MessageBox` | `wxMessageBox` |
| `ShellExecute` (URL) | `wxLaunchDefaultBrowser` |
| `SetCurrentDir` | `wxSetWorkingDirectory` |
| `WinExec` / `CreateProcess` | `wxExecute` |
| `TImageList` + machine bitmaps | `wxImageList` + embedded BMP resources |

---

## Incomplete / Known Gaps

(None known — all critical functionality is in place. Audio playback now uses miniaudio v0.11.22, integrated as a single-header library at `wxwidgets_port/miniaudio.h` with no extra DLLs or vcpkg dependencies.)

---

## Build Setup

**Prerequisites (installed):**
- CMake 4.1.2
- MSVC (Visual Studio Build Tools 2022)
- wxWidgets 3.3.1 via vcpkg at `C:\vcpkg`

**Build:**
```bat
cd wxwidgets_port\build
cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Debug
```

Or use `build.bat` (sets `VCPKG_POWERSHELL_PATH=powershell.exe` to suppress the vcpkg pwsh warning).

**Output:** `wxwidgets_port\build\Debug\otla.exe`
