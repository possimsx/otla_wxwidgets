# Build Instructions

## Prerequisites (All Installed ✅)

- ✅ **CMake** 4.1.2 - Installed via winget
- ✅ **C++ Compiler** - MSVC (Visual Studio Build Tools 2022) - Found automatically
- ✅ **wxWidgets** 3.3.1 - Installed via vcpkg
- ✅ **vcpkg** - Installed at `C:\vcpkg`

## Building the Project

### Option 1: Using the build script
```bash
.\build.bat
```

### Option 2: Manual build
```bash
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . --config Release
```

### Option 3: Using Visual Studio
1. Open the `build` folder in Visual Studio
2. Open `otla.sln`
3. Build → Build Solution (or press F7)

## Build Output

The executable will be at:
- Debug: `build\Debug\otla.exe`
- Release: `build\Release\otla.exe`

## Notes

- The vcpkg toolchain file is required for CMake to find wxWidgets
- All dependencies are installed and configured

## vcpkg pwsh.exe Warning

vcpkg's post-build DLL copy step tries `pwsh.exe` (PowerShell Core) and falls back with an error message if only `powershell.exe` (Windows PowerShell) is available. The executable still builds correctly — this is a cosmetic warning only.

To silence it, set the environment variable before building:
```bat
set VCPKG_POWERSHELL_PATH=powershell.exe
```
Or set it permanently in Windows system environment variables. The `build.bat` script sets this automatically.

