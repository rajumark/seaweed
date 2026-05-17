# Debug Build Scripts

This directory contains scripts to build the ADBKing project in debug mode on Windows, replicating the CI/CD environment locally.

## Available Scripts

### `build_debug.bat` (Batch File)
- **Usage**: Double-click or run from command prompt
- **Requirements**: Git, Visual Studio 2022 Build Tools, CMake
- **What it does**: Sets up vcpkg, installs SDL2 with static linking, and builds the project in debug mode

### `build_debug.ps1` (PowerShell)
- **Usage**: Right-click → "Run with PowerShell" or run from PowerShell
- **Requirements**: Same as batch file
- **What it does**: Same as batch file but with better error handling and colored output

## Prerequisites

Before running the scripts, make sure you have:

1. **Git** installed and accessible from command line
2. **Visual Studio 2022 Build Tools** (or full Visual Studio 2022)
3. **CMake** (usually comes with Visual Studio)

## How to Use

### Option 1: Batch File (Recommended for beginners)
1. Double-click `build_debug.bat`
2. The script will automatically:
   - Clone vcpkg if not present
   - Install SDL2 with static linking
   - Configure CMake for debug build
   - Build the application
   - Ask if you want to run the application

### Option 2: PowerShell (Recommended for developers)
1. Right-click `build_debug.ps1`
2. Select "Run with PowerShell"
3. Follow the same process as above

## What the Scripts Do

The scripts replicate the exact same environment as the GitHub Actions CI/CD:

1. **vcpkg Setup**: Clones and bootstraps vcpkg for package management
2. **SDL2 Installation**: Installs SDL2 with static linking (`sdl2:x64-windows-static`)
3. **Environment Variables**: Sets up the same environment variables as CI/CD
4. **CMake Configuration**: Uses the same CMake flags but for Debug instead of Release
5. **Build Process**: Builds the application in debug mode with static runtime linking

## Key Differences from CI/CD

- **Build Type**: Debug instead of Release
- **Runtime Library**: `MultiThreadedDebug` instead of `MultiThreaded`
- **Compiler Flags**: `/MTd` instead of `/MT` (debug static runtime)

## Output

After successful build:
- Executable: `build\Debug\imgui_app.exe`
- File size will be larger than release build (debug symbols included)
- Static linking means no external DLL dependencies

## Troubleshooting

### "CMake not found"
- Install Visual Studio 2022 Build Tools
- Make sure CMake is in your PATH

### "Git not found"
- Install Git from https://git-scm.com/
- Make sure git is in your PATH

### "SDL2 installation failed"
- Check your internet connection
- Try running the script as administrator
- Check if antivirus is blocking the download

### "Build failed"
- Check the error messages in the console
- Make sure you have enough disk space
- Try cleaning the build directory manually

## Notes

- The scripts create a `vcpkg` directory in your project root
- The scripts create a `build` directory for the build output
- Static linking means the executable will be larger but won't need external DLLs
- Debug builds include debug symbols for better debugging experience

## Integration with CI/CD

These scripts are designed to work alongside the existing CI/CD pipeline without breaking it:
- They use the same vcpkg setup as CI/CD
- They use the same CMake configuration approach
- They don't modify any project files
- They can be safely committed to the repository 