# wxWidgets Port of O.T.L.A.

This directory contains the wxWidgets port of the O.T.L.A. application from Borland C++ Builder 6.
MSX part of code has been improved, hecen the 2.2.5 version.
Heavily assisted by AI.

## Building

### Prerequisites

1. **wxWidgets** (3.1.x or 3.2.x)
   - Download from https://www.wxwidgets.org/
   - Or use package manager:
     - Windows: `vcpkg install wxwidgets`
     - Linux: `sudo apt-get install libwxgtk3.0-dev` (Ubuntu/Debian)
     - macOS: `brew install wxwidgets`

2. **CMake** (3.16 or later)

3. **C++ Compiler**
   - Windows: MSVC, MinGW, or Clang
   - Linux: GCC or Clang
   - macOS: Clang (Xcode)

### Build Steps

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ../wxwidgets_port

# Build
cmake --build .

# On Windows with MSVC:
cmake --build . --config Release

# Run
./otla  # Linux/macOS
otla.exe  # Windows
```

### Finding wxWidgets

If CMake can't find wxWidgets automatically:

```bash
# Windows (vcpkg)
cmake -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake ../wxwidgets_port

# Linux (custom path)
cmake -DwxWidgets_ROOT_DIR=/usr/lib/wx/config/gtk3-unicode-3.1 ../wxwidgets_port

# macOS (Homebrew)
cmake -DwxWidgets_ROOT_DIR=/usr/local/opt/wxwidgets ../wxwidgets_port
```

## Project Structure

```
wxwidgets_port/
├── CMakeLists.txt          # Build configuration
├── main.cpp                # Application entry point
├── MainFrame.h/cpp         # Main window (ported)
├── AboutDialog.h/cpp       # About dialog
├── Dialogs.h/cpp           # Other dialogs (placeholders)
└── README.md               # This file
```

## Porting Status

### ✅ Completed
- Project structure and CMake build system (vcpkg + MSVC)
- All 9 original forms ported as code-based wxWidgets dialogs
- All core logic files linked unchanged (`funciones`, `sbb`, `wav2`, `loaders`, `zxfiles`, etc.)
- `AnsiString` → `wxString`, `TIniFile` → `wxFileConfig`, Borland pragmas removed
- WAV file reading (MMIO → standard fread RIFF parser)
- Waveform visualization (`wxDC`)
- Machine logo BMP embedding via Python script at build time

See `MIGRATION_STATUS.md` for the full detailed status.

## Dependencies

- **wxWidgets 3.3.1**: GUI framework (via vcpkg)
- **miniaudio 0.11.22**: Audio playback (single-header, vendored at `wxwidgets_port/miniaudio.h`, no extra setup)

## License

Same as original O.T.L.A. project.


