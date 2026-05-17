# Setup Guide for ImGui Project

## Prerequisites Installation

### 1. Install Visual Studio Build Tools (Recommended)
1. Go to https://visualstudio.microsoft.com/downloads/
2. Download "Build Tools for Visual Studio 2022" (free)
3. Run the installer
4. Select "C++ build tools" workload
5. Install

### 2. Install SDL2 Development Libraries
You have several options:

#### Option A: Using vcpkg (Recommended)
```powershell
# Install vcpkg if you don't have it
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install SDL2
.\vcpkg install sdl2:x64-windows
```

#### Option B: Manual Installation
1. Download SDL2 development libraries from https://www.libsdl.org/download-2.0.php
2. Extract to `C:\SDL2\` or `C:\tools\sdl2\current\`
3. Make sure the structure is:
   ```
   C:\SDL2\
   ├── include\SDL2\
   │   └── SDL.h
   └── lib\x64\
       └── SDL2.lib
   ```

### 3. Configure CMake
After installing the tools:
1. In VS Code, press `Ctrl+Shift+P`
2. Type "CMake: Configure"
3. Select your newly installed compiler kit
4. The project should now build successfully

## Building the Project

Once everything is installed:
1. Open the project in VS Code
2. Press `Ctrl+Shift+P` and run "CMake: Configure"
3. Select your compiler kit when prompted
4. Use the play button or run "CMake: Build" to build the project

## Troubleshooting

### If CMake can't find SDL2:
- Make sure SDL2 is installed in one of the paths listed in CMakeLists.txt
- Or set the `SDL2_DIR` environment variable to point to your SDL2 installation

### If you get linker errors:
- Ensure you're using the correct architecture (x64 vs x86)
- Make sure all SDL2 DLLs are in the same directory as your executable

### If the play button doesn't work:
- Check the Output panel for CMake errors
- Make sure all dependencies are properly installed 