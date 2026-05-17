# Static Linking Fix for VCRUNTIME140.dll Error

## Problem
The application was failing with the error:
```
The code execution cannot proceed because VCRUNTIME140.dll was not found.
```

This happens when an application is built with dynamic runtime linking, which requires the Visual C++ Redistributable to be installed on the target system.

## Solution
The application has been configured to use static runtime linking, which embeds the Visual C++ runtime libraries directly into the executable.

### Changes Made

#### 1. CMakeLists.txt Updates
- **Static Runtime Configuration**: Set `CMAKE_MSVC_RUNTIME_LIBRARY` to `"MultiThreaded$<$<CONFIG:Debug>:Debug>"`
- **Compiler Flags**: Force `/MT` (static) and `/MTd` (debug static) flags
- **Target Properties**: Added `MSVC_RUNTIME_LIBRARY` property to the executable target
- **SDL2 Static Linking**: Configured to prefer static SDL2 libraries on Windows

#### 2. GitHub Actions Workflow Updates
- **SDL2 Static Build**: Changed from `sdl2:x64-windows` to `sdl2:x64-windows-static`
- **CMake Configuration**: Added explicit `/MT` flags to CMake command
- **Verification**: Added `dumpbin` checks to verify static linking
- **Installer**: Removed SDL2.dll bundling since static linking is used

#### 3. Verification Steps
The workflow now includes verification steps:
- File size check (static linking typically results in larger executables)
- `dumpbin /directives` to check for MSVCRT references
- `dumpbin /dependents` to check for VCRUNTIME140.dll dependencies

### Testing Locally
Run the test script to verify static linking:
```powershell
.\test_static_linking.ps1
```

### Expected Results
- ✅ No VCRUNTIME140.dll dependency
- ✅ No MSVCRT references in executable
- ✅ Larger executable size (due to embedded runtime)
- ✅ Executable runs without Visual C++ Redistributable

### Benefits
1. **No Dependencies**: Application runs without requiring Visual C++ Redistributable
2. **Portable**: Single executable that works on any Windows system
3. **Reliable**: No runtime library version conflicts
4. **Simplified Deployment**: No need to bundle or install additional runtime libraries

### Trade-offs
1. **Larger Executable**: Static linking increases file size
2. **No Runtime Updates**: Can't benefit from runtime library updates
3. **Security**: Can't receive security updates to runtime libraries

### Verification Commands
```powershell
# Check for VCRUNTIME140.dll dependency
dumpbin /dependents imgui_app.exe | findstr VCRUNTIME140

# Check for MSVCRT references
dumpbin /directives imgui_app.exe | findstr MSVCRT

# Check file size (should be larger with static linking)
Get-Item imgui_app.exe | Select-Object Length
``` 