@echo off
setlocal enabledelayedexpansion
REM Build script for O.T.L.A. wxWidgets port

echo Building O.T.L.A. wxWidgets port...
echo.

REM Set PowerShell path for vcpkg (use Windows PowerShell instead of PowerShell Core)
set VCPKG_POWERSHELL_PATH=powershell.exe

REM Check for vcpkg
if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    echo Using vcpkg toolchain...
    set VCPKG_TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
) else (
    echo vcpkg not found. Trying without toolchain...
    set VCPKG_TOOLCHAIN=
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure only if CMakeCache.txt doesn't exist
REM This prevents unnecessary project file regeneration which causes full rebuilds
if not exist "CMakeCache.txt" (
    echo Configuring CMake (first time)...
    cmake .. %VCPKG_TOOLCHAIN% -DCMAKE_PREFIX_PATH=C:/vcpkg/installed/x64-windows
    if errorlevel 1 (
        echo.
        echo ERROR: CMake configuration failed!
        echo.
        echo Please install wxWidgets:
        echo   1. Using vcpkg: vcpkg install wxwidgets
        echo   2. Or download from: https://www.wxwidgets.org/downloads/
        echo.
        pause
        exit /b 1
    )
) else (
    echo Skipping CMake configuration (already configured)...
    echo Note: Run 'cmake ..' manually if you changed CMakeLists.txt
)

REM Build
echo.
echo Building...
cmake --build . --config Release
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable: build\Release\otla.exe
echo.
pause
