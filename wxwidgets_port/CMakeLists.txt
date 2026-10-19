cmake_minimum_required(VERSION 3.16)

# Static vcpkg triplet (e.g. x64-windows-static): also link the MSVC runtime
# statically so the result is a single self-contained otla.exe
if(VCPKG_TARGET_TRIPLET MATCHES "-static$")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(otla VERSION 2.2.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add /FS flag for parallel builds on Windows (must be before add_executable)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS")
endif()

# Auto-detect vcpkg and set CMAKE_PREFIX_PATH if using vcpkg toolchain
if(CMAKE_TOOLCHAIN_FILE)
    get_filename_component(VCPKG_ROOT "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(VCPKG_ROOT "${VCPKG_ROOT}" DIRECTORY)
    get_filename_component(VCPKG_ROOT "${VCPKG_ROOT}" DIRECTORY)
    if(EXISTS "${VCPKG_ROOT}/installed/x64-windows")
        list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")
    endif()
endif()

# Find wxWidgets (try CONFIG first for vcpkg, then MODULE for system install)
find_package(wxWidgets QUIET CONFIG COMPONENTS core base adv html net)
if(NOT wxWidgets_FOUND)
    # Try MODULE mode (for system-installed wxWidgets)
    find_package(wxWidgets QUIET COMPONENTS core base adv html net)
    if(wxWidgets_FOUND)
        include(${wxWidgets_USE_FILE})
    endif()
endif()

if(NOT wxWidgets_FOUND)
    message(FATAL_ERROR "wxWidgets not found! Please install wxWidgets:\n"
        "  - Using vcpkg: vcpkg install wxwidgets\n"
        "  - Or download from: https://www.wxwidgets.org/downloads/\n"
        "  - Or set wxWidgets_ROOT_DIR to wxWidgets installation directory")
endif()

# Source files - Core logic (mostly portable)
set(CORE_SOURCES
    ../source/funciones.cpp
    ../source/sbb.cpp
    ../source/wav2.cpp
    ../source/loaders.cpp
    ../source/zxfiles.cpp
    ../source/81files.cpp
    ../source/cpcfiles.cpp
    ../source/msxfiles.cpp
    ../source/signaltest.cpp
)

# MSX TSX generator (direct SBB -> TSX, no WAV intermediate)
set(TSX_SOURCES
    tsx/sbb2tsx.cpp
)

# Source files - GUI (to be ported)
set(GUI_SOURCES
    main.cpp
    MainFrame.cpp
    AboutDialog.cpp
    SettingsDialog.cpp
    DirDialog.cpp
    ModifyDialog.cpp
    InputDialog.cpp
    BatchDialog.cpp
    AdjustDialog.cpp
    WavMonitorDialog.cpp
    Dialogs.cpp
)

# Header files
set(HEADERS
    MainFrame.h
    AboutDialog.h
    SettingsDialog.h
    DirDialog.h
    ModifyDialog.h
    InputDialog.h
    BatchDialog.h
    AdjustDialog.h
    WavMonitorDialog.h
    Dialogs.h
    ../source/funciones.h
    ../source/sbb.h
    ../source/wav2.h
    ../source/loaders.h
    ../source/zxfiles.h
    ../source/81files.h
    ../source/cpcfiles.h
    ../source/msxfiles.h
    ../source/signaltest.h
)

# Windows resource file (embeds app icon)
if(WIN32)
    set(RC_SOURCES otla.rc)
else()
    set(RC_SOURCES "")
endif()

# Create executable
add_executable(${PROJECT_NAME} ${GUI_SOURCES} ${CORE_SOURCES} ${TSX_SOURCES} ${RC_SOURCES})

# Add /FS flag for parallel builds on Windows
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS")
endif()

# Link wxWidgets (modern CMake targets)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    wx::core 
    wx::base 
    wx::adv 
    wx::html 
    wx::net
)

# vcpkg applocal is enabled to automatically copy DLLs
# vcpkg tries pwsh.exe first, then falls back to powershell.exe
# To avoid the error message, we create a pwsh.exe wrapper
# See setup_pwsh_wrapper.bat for the solution

# Windows specific
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Generate embedded BMP resources
# BMP files are located relative to wxwidgets_port directory
set(BMP_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../source/bmp)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/embedded_bmp_resources.h
               ${CMAKE_BINARY_DIR}/embedded_bmp_resources.cpp
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/embed_bmp.py
                ${BMP_SOURCE_DIR}
                ${CMAKE_BINARY_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embed_bmp.py
                ${BMP_SOURCE_DIR}/zxspectrum.bmp
                ${BMP_SOURCE_DIR}/amstred.bmp
                ${BMP_SOURCE_DIR}/msx.bmp
                ${BMP_SOURCE_DIR}/zx81.bmp
                ${BMP_SOURCE_DIR}/81.bmp
                ${BMP_SOURCE_DIR}/otla_logo.bmp
        COMMENT "Generating embedded BMP resources"
        VERBATIM
    )
    # Add generated source to executable
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/embedded_bmp_resources.cpp)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR})
else()
    message(WARNING "Python3 not found - falling back to file-based BMP loading")
    # Fallback: copy resources if Python is not available
    file(COPY ${BMP_SOURCE_DIR} DESTINATION ${CMAKE_BINARY_DIR})
endif()


