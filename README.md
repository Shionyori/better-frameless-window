# Qt VSCode Starter

This is a ready-to-use Qt C++ project template for VS Code. It comes pre-configured with CMake build settings, debugging configurations, and code completion support, suitable for Windows, Linux, and macOS.

## Prerequisites

1.  **Install Qt**: Ensure Qt 6 is installed.
    *   **Windows**: Recommended to use the MSVC version (e.g., `msvc2019_64` or `msvc2022_64`).
    *   **Linux**: Install via apt (`sudo apt install qt6-base-dev`) or use the Qt Online Installer.
    *   **macOS**: Install via Homebrew (`brew install qt`) or use the Qt Online Installer.

2.  **Set Environment Variables**:
    *   You need to set the `QT_DIR` environment variable to point to your specific Qt build kit location.
    *   **Windows Example**: `D:\Qt\6.8.3\msvc2022_64`
    *   **Linux/macOS Example**: `~/Qt/6.8.3/gcc_64` or `/usr/local/opt/qt` (Homebrew)
    *   Alternatively, you can create a `CMakeUserPresets.json` file to override the `QT_DIR` value.

3.  **Install VS Code Extensions**:
    *   C/C++ (ms-vscode.cpptools)
    *   CMake Tools (ms-vscode.cmake-tools)
    *   Qt Configure (Optional, for UI design helper)

## Features & Highlights

*   **Zero-Config Qt Detection**:
    *   **Solves "Qt not found"**: Simply set `QT_DIR` environment variable, or use `CMakeUserPresets.json`.
    *   The intelligent `CMakeLists.txt` automatically appends `QT_DIR` to `CMAKE_PREFIX_PATH`, so you don't need to manually modify build scripts for different machines.

*   **One-Click Debugging (Automatic DLL Deployment)**:
    *   **Solves "Missing DLL" errors on Windows**: No more manually copying `Qt6Core.dll` or modifying system `%PATH%`.
    *   **Automated `windeployqt`**: The `CMakeLists.txt` automatically runs `windeployqt` after build. It deploys the exact required Qt DLLs and plugins to your build folder, making the executable completely standalone and ready to debug.
    *   **macOS Bundle**: Automatically runs `macdeployqt` to create a proper `.app` bundle with embedded Frameworks.

*   **Multi-Platform Ready**:
    *   Pre-configured **CMake Presets** for Windows (MSVC), Linux (GCC/Ninja), and macOS (Clang/Ninja).
    *   Consistent build commands across all OSs.

*   **VS Code Optimized**:
    *   **`launch.json`**: Pre-configured debuggers (`cppvsdbg` for Windows, `cppdbg` for Unix).
    *   **`extensions.json`**: Auto-recommends necessary plugins.
    *   **`settings.json`**: Pre-configured CMake Tools environment.
    *   **IntelliSense**: `c_cpp_properties.json` is configured to use CMake Tools provider for accurate code completion.

## How to Use

1.  Open this project folder in VS Code.
2.  Change the `TARGET_NAME` variable in `CMakeLists.txt` to your actual project name.
3.  Set the `QT_DIR` environment variable to point to your Qt installation, or create a `CMakeUserPresets.json` with CacheVariables to override it.
4.  Open the Command Palette (Ctrl+Shift+P) and select "CMake: Select Configure Preset" to choose the appropriate preset for your platform (e.g., `windows-debug`, `linux-debug`, `macos-debug`).
5.  Run "CMake: Configure" to generate the build files.
6.  Run "CMake: Build" to compile the project.
7.  Run "CMake: Debug" to start debugging. The Qt window should appear.