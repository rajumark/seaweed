# ADBKing Debug Build Script (PowerShell Version)
# This script replicates the CI/CD setup locally for debug builds

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "ADBKing Debug Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "ERROR: CMakeLists.txt not found!" -ForegroundColor Red
    Write-Host "Please run this script from the project root directory." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if vcpkg exists, if not clone it
if (-not (Test-Path "vcpkg")) {
    Write-Host "Setting up vcpkg for SDL2 management..." -ForegroundColor Yellow
    git clone https://github.com/Microsoft/vcpkg.git
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to clone vcpkg!" -ForegroundColor Red
        Write-Host "Make sure git is installed and you have internet access." -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
}

# Bootstrap vcpkg if needed
if (-not (Test-Path "vcpkg\vcpkg.exe")) {
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Yellow
    & "vcpkg\bootstrap-vcpkg.bat"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to bootstrap vcpkg!" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
}

# Install SDL2 with static linking (same as CI/CD)
Write-Host "Installing SDL2 with static linking via vcpkg..." -ForegroundColor Yellow
& "vcpkg\vcpkg.exe" install sdl2:x64-windows-static
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to install SDL2!" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Set up environment variables (same as CI/CD)
$env:VCPKG_ROOT = (Resolve-Path "vcpkg").Path
$env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"

Write-Host ""
Write-Host "Environment setup:" -ForegroundColor Green
Write-Host "VCPKG_ROOT: $env:VCPKG_ROOT" -ForegroundColor Gray
Write-Host "CMAKE_TOOLCHAIN_FILE: $env:CMAKE_TOOLCHAIN_FILE" -ForegroundColor Gray
Write-Host ""

# Clean and create build directory
if (Test-Path "build") {
    Write-Host "Cleaning previous build..." -ForegroundColor Yellow
    Remove-Item "build" -Recurse -Force
}
New-Item -ItemType Directory -Path "build" -Force | Out-Null
Set-Location "build"

# Configure CMake for Debug build (same flags as CI/CD but Debug instead of Release)
Write-Host "Configuring CMake for Debug build..." -ForegroundColor Yellow
$cmakeArgs = @(
    "..",
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DCMAKE_TOOLCHAIN_FILE=`"$env:CMAKE_TOOLCHAIN_FILE`"",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebug",
    "-DCMAKE_CXX_FLAGS=/MTd",
    "-DCMAKE_C_FLAGS=/MTd",
    "-DCMAKE_PREFIX_PATH=`"$env:VCPKG_ROOT\installed\x64-windows-static`""
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    Set-Location ".."
    Read-Host "Press Enter to exit"
    exit 1
}

# Build the application in Debug mode
Write-Host ""
Write-Host "Building application in Debug mode..." -ForegroundColor Yellow
& cmake --build . --config Debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Set-Location ".."
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if executable was created
if (Test-Path "Debug\imgui_app.exe") {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "✅ BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    
    $exePath = (Resolve-Path "Debug\imgui_app.exe").Path
    Write-Host "Executable location: $exePath" -ForegroundColor Cyan
    
    # Show file size
    $fileSize = (Get-Item $exePath).Length
    $sizeKB = [math]::Round($fileSize / 1KB, 2)
    Write-Host "File size: $sizeKB KB" -ForegroundColor Gray
    
    Write-Host ""
    Write-Host "Debug build completed successfully!" -ForegroundColor Green
    Write-Host "You can now run the application in debug mode." -ForegroundColor Green
    Write-Host ""
    
    # Ask if user wants to run the application
    $runApp = Read-Host "Do you want to run the application now? (y/n)"
    if ($runApp -eq "y" -or $runApp -eq "Y") {
        Write-Host "Starting application..." -ForegroundColor Yellow
        Start-Process -FilePath $exePath
    }
    
} else {
    Write-Host "ERROR: Executable not found after build!" -ForegroundColor Red
    Write-Host "Expected location: $((Get-Location).Path)\Debug\imgui_app.exe" -ForegroundColor Red
    Set-Location ".."
    Read-Host "Press Enter to exit"
    exit 1
}

Set-Location ".."
Write-Host ""
Write-Host "Build script completed successfully!" -ForegroundColor Green
Read-Host "Press Enter to exit" 