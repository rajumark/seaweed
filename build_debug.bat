@echo off
setlocal enabledelayedexpansion

echo ========================================
echo ADBKing Debug Build Script (Optimized)
echo ========================================
echo.

:: Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found!
    echo Please run this script from the project root directory.
    pause
    exit /b 1
)

:: Check if CMake is available
where cmake >nul 2>&1
if errorlevel 1 (
    :: Try to find CMake in common installation paths
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "CMAKE_PATH=C:\Program Files\CMake\bin\cmake.exe"
        echo Found CMake at: %CMAKE_PATH%
    ) else if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" (
        set "CMAKE_PATH=C:\Program Files (x86)\CMake\bin\cmake.exe"
        echo Found CMake at: %CMAKE_PATH%
    ) else (
        echo ERROR: CMake not found in PATH or common installation directories!
        echo Please install CMake and add it to your PATH, or run from Visual Studio Developer Command Prompt.
        echo.
        echo You can download CMake from: https://cmake.org/download/
        echo Or use Visual Studio Installer to install CMake.
        pause
        exit /b 1
    )
) else (
    set "CMAKE_PATH=cmake"
)

:: Check if vcpkg exists, if not clone it
if not exist "vcpkg" (
    echo Setting up vcpkg for SDL2 management...
    git clone https://github.com/Microsoft/vcpkg.git
    if errorlevel 1 (
        echo ERROR: Failed to clone vcpkg!
        echo Make sure git is installed and you have internet access.
        pause
        exit /b 1
    )
    echo ✅ vcpkg cloned successfully
) else (
    echo ✅ vcpkg already exists, skipping clone
)

:: Bootstrap vcpkg if needed
if not exist "vcpkg\vcpkg.exe" (
    echo Bootstrapping vcpkg...
    call vcpkg\bootstrap-vcpkg.bat
    if errorlevel 1 (
        echo ERROR: Failed to bootstrap vcpkg!
        pause
        exit /b 1
    )
    echo ✅ vcpkg bootstrapped successfully
) else (
    echo ✅ vcpkg already bootstrapped, skipping
)

:: Check if SDL2 is already installed
echo Checking SDL2 installation...
call vcpkg\vcpkg list sdl2:x64-windows-static >nul 2>&1
if errorlevel 1 (
    echo Installing SDL2 with static linking via vcpkg...
    call vcpkg\vcpkg install sdl2:x64-windows-static
    if errorlevel 1 (
        echo ERROR: Failed to install SDL2!
        pause
        exit /b 1
    )
    echo ✅ SDL2 installed successfully
) else (
    echo ✅ SDL2 already installed, skipping installation
)

:: Set up environment variables (same as CI/CD)
set VCPKG_ROOT=%CD%\vcpkg
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

echo.
echo Environment setup:
echo VCPKG_ROOT: %VCPKG_ROOT%
echo CMAKE_TOOLCHAIN_FILE: %CMAKE_TOOLCHAIN_FILE%
echo.

:: Check if build directory exists and has CMake cache
set NEED_CONFIGURE=1
if exist "build\CMakeCache.txt" (
    echo ✅ Found existing CMake cache, checking if configuration is up to date...
    
    :: Check if the cache contains our expected configuration
    findstr /C:"CMAKE_BUILD_TYPE:STRING=Debug" build\CMakeCache.txt >nul 2>&1
    if not errorlevel 1 (
        findstr /C:"CMAKE_TOOLCHAIN_FILE:FILEPATH=" build\CMakeCache.txt >nul 2>&1
        if not errorlevel 1 (
            echo ✅ CMake configuration is up to date, skipping configuration
            set NEED_CONFIGURE=0
        )
    )
)

:: Create build directory if it doesn't exist
if not exist "build" mkdir build
cd build

:: Configure CMake only if needed
if !NEED_CONFIGURE!==1 (
    echo Configuring CMake for Debug build...
    "%CMAKE_PATH%" .. -G "Visual Studio 17 2022" -A x64 ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" ^
        -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreadedDebug" ^
        -DCMAKE_CXX_FLAGS="/MTd" ^
        -DCMAKE_C_FLAGS="/MTd" ^
        -DCMAKE_PREFIX_PATH="%VCPKG_ROOT%\installed\x64-windows-static"

    if errorlevel 1 (
        echo ERROR: CMake configuration failed!
        cd ..
        pause
        exit /b 1
    )
    echo ✅ CMake configuration completed
) else (
    echo Using existing CMake configuration
)

:: Build the application in Debug mode with parallel compilation
echo.
echo Building application in Debug mode with parallel compilation...

:: Get number of CPU cores for parallel compilation
for /f "tokens=2 delims==" %%a in ('wmic cpu get NumberOfCores /value 2^>nul ^| find "="') do set CORES=%%a
if not defined CORES set CORES=4

echo Using !CORES! parallel jobs for compilation...

"%CMAKE_PATH%" --build . --config Debug --parallel !CORES!

if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

:: Check if executable was created
if exist "Debug\imgui_app.exe" (
    echo.
    echo ========================================
    echo ✅ BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Executable location: %CD%\Debug\imgui_app.exe
    
    :: Show file size
    for %%A in ("Debug\imgui_app.exe") do (
        set /a size=%%~zA/1024
        echo File size: !size! KB
    )
    
    echo.
    echo Debug build completed successfully!
    echo You can now run the application in debug mode.
    echo.
    
    :: Run the application directly
    echo Starting application...
    
    :: Copy font file to build directory for runtime access
    echo Copying JetBrains Mono font to build directory...
    copy "..\imgui\misc\fonts\JetBrainsMono.ttf" "Debug\JetBrainsMono.ttf" >nul 2>&1
    if exist "Debug\JetBrainsMono.ttf" (
        echo ✅ Font file copied successfully
    ) else (
        echo ⚠️  Warning: Font file copy failed
    )
    
    start "" "Debug\imgui_app.exe"
    
) else (
    echo ERROR: Executable not found after build!
    echo Expected location: %CD%\Debug\imgui_app.exe
    pause
    exit /b 1
)

cd ..
echo.
echo Build script completed successfully!
pause 