## Build and Run the App 
 
### Method 1: Using CMake (Recommended)

1. **Create build directory and configure**:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   ```

2. **Build the project**:
   ```bash
   make
   ```

3. **Run the application**:
   ```bash
   ./imgui_app
   ```

### Method 2: Using g++ directly

```bash
g++ \
  src/main.cpp \
  src/splash.cpp \
  src/setup.cpp \
  src/home.cpp \
  imgui/*.cpp \
  imgui/backends/imgui_impl_sdl2.cpp \
  imgui/backends/imgui_impl_opengl3.cpp \
  -Iimgui \
  -Iimgui/backends \
  `sdl2-config --cflags --libs` \
  -framework OpenGL \
  -std=c++17 \
  -o imgui_app
```

### Run the app

```bash
./imgui_app
```

### Or can run in left side menu vscode
-> Play -> Run and Debug

## Prerequisites

- **SDL2**: Install via Homebrew (`brew install sdl2`)
- **CMake**: Should be available on macOS by default
- **C++ compiler**: AppleClang (already available)

## Quick Commands for Future Runs

Once built with CMake, you can simply:
```bash
cd build
./imgui_app
```

Or to rebuild after making changes:
```bash
cd build
make
./imgui_app
```

## CI/CD Pipeline

This project includes a GitHub Actions workflow for automated builds and releases.

### Automated Builds

The CI/CD pipeline automatically:
- Builds universal macOS binaries (x86_64 + arm64)
- Runs on every push to `main`/`master` branch
- Creates build artifacts for download
- Prints the output file path in the logs

### Workflow Details

The workflow (`.github/workflows/ci-cd.yml`) performs the following steps:

1. **Setup**: Installs dependencies (CMake, SDL2)
2. **Build x86_64**: Compiles for Intel Macs
3. **Build arm64**: Compiles for Apple Silicon Macs
4. **Create Universal Binary**: Combines both architectures using `lipo`
5. **Verify**: Checks the universal binary architecture
6. **Output**: Prints the final file path and uploads as artifact

### Accessing Build Artifacts

After a successful workflow run:
- Download the `imgui-app-universal` artifact from the Actions tab
- The universal binary will be named `imgui_app`
- File path is printed in the workflow logs: `${{ github.workspace }}/dist/imgui_app`

### Manual Trigger

You can manually trigger the workflow:
1. Go to the Actions tab in your GitHub repository
2. Select "CI/CD - Universal macOS Build"
3. Click "Run workflow"

### Release Automation

When you create a Git tag, the workflow automatically:
- Creates a GitHub release
- Attaches the universal binary
- Includes architecture and file size information

## Windows Build

### Automated Windows MSI Creation

The project includes a Windows-specific workflow (`.github/workflows/win.yml`) that:

1. **Builds Windows executable** using Visual Studio 2022
2. **Creates MSI installer** using WiX Toolset
3. **Static runtime linking** to avoid MSVCP140.dll dependency
4. **Includes SDL2.dll** in the installer
5. **Deploys to private repository** for distribution

### MSVCP140.dll and VCRUNTIME140.dll Fix

The Windows build is configured to use **static runtime linking** to eliminate the need for Visual C++ Redistributable on target systems. This prevents the common errors:

```
The code execution cannot proceed because MSVCP140.dll was not found.
The code execution cannot proceed because VCRUNTIME140.dll was not found.
```
 
**Key Configuration:**
- `CMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded"` in CMakeLists.txt
- `/MT` compiler flags for Release builds
- `/MTd` compiler flags for Debug builds
- Static linking of both C and C++ runtime libraries
- Verification step in CI to confirm static linking

### Windows MSI Features

The generated MSI installer includes:
- **Main executable** (`imgui_app.exe`)
- **SDL2.dll** (required dependency)
- **Start menu shortcut**
- **Registry entries** for uninstall tracking
- **Automatic installation** to Program Files

## Platform Tools Downloads 

Download platform tools for different operating systems:

- **Windows**: [platform-tools-windows.zip](https://github.com/rajumark/adbcontent/blob/main/platform-tools-windows.zip)
- **macOS**: [platform-tools-macos.zip](https://github.com/rajumark/adbcontent/blob/main/platform-tools-macos.zip)
- **Linux**: [platform-tools-linux.zip](https://github.com/rajumark/adbcontent/blob/main/platform-tools-linux.zip)