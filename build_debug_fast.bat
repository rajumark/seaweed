@echo off
setlocal enabledelayedexpansion

echo ========================================
echo ADBKing Ultra-Fast Debug Build
echo ========================================
echo.

:: Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found!
    echo Please run this script from the project root directory.
    pause
    exit /b 1
)

:: Quick CMake check
where cmake >nul 2>&1
if errorlevel 1 (
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "CMAKE_PATH=C:\Program Files\CMake\bin\cmake.exe"
    ) else if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" (
        set "CMAKE_PATH=C:\Program Files (x86)\CMake\bin\cmake.exe"
    ) else (
        echo ERROR: CMake not found!
        pause
        exit /b 1
    )
) else (
    set "CMAKE_PATH=cmake"
)

:: Set up environment variables
set VCPKG_ROOT=%CD%\vcpkg
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

:: Create build directory if it doesn't exist
if not exist "build" mkdir build
cd build

:: Check if we need to configure (only if CMakeCache.txt doesn't exist or is invalid)
set NEED_CONFIGURE=1
if exist "CMakeCache.txt" (
    findstr /C:"CMAKE_BUILD_TYPE:STRING=Debug" CMakeCache.txt >nul 2>&1
    if not errorlevel 1 (
        echo ✅ Using existing CMake configuration
        set NEED_CONFIGURE=0
    )
)

:: Configure only if absolutely necessary
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
)

:: Get CPU cores for maximum parallel compilation
for /f "tokens=2 delims==" %%a in ('wmic cpu get NumberOfCores /value 2^>nul ^| find "="') do set CORES=%%a
if not defined CORES set CORES=8

echo Building with !CORES! parallel jobs...

:: Build with maximum optimization
"%CMAKE_PATH%" --build . --config Debug --parallel !CORES! --verbose

if errorlevel 1 (
    echo ERROR: Build failed!
    cd ..
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
    echo Executable: %CD%\Debug\imgui_app.exe
    
    :: Show file size
    for %%A in ("Debug\imgui_app.exe") do (
        set /a size=%%~zA/1024
        echo Size: !size! KB
    )
    
    :: Auto-run the application
    echo.
    echo Starting application...
    start "" "Debug\imgui_app.exe"
    
) else (
    echo ERROR: Executable not found!
    echo Expected: %CD%\Debug\imgui_app.exe
    cd ..
    pause
    exit /b 1
)

cd ..
echo.
echo Fast build completed! 