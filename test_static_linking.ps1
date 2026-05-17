# Test script to verify static linking
Write-Host "Testing static linking configuration..." -ForegroundColor Green

# Create build directory
if (Test-Path "build") {
    Remove-Item "build" -Recurse -Force
}
mkdir build
cd build

# Configure with static linking
Write-Host "Configuring CMake with static linking..." -ForegroundColor Yellow
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" -DCMAKE_CXX_FLAGS="/MT" -DCMAKE_C_FLAGS="/MT"

# Build the application
Write-Host "Building application..." -ForegroundColor Yellow
cmake --build . --config Release

# Check if executable exists
$exePath = "Release\imgui_app.exe"
if (Test-Path $exePath) {
    Write-Host "✅ Executable built successfully" -ForegroundColor Green
    
    # Check file size
    $fileSize = (Get-Item $exePath).Length
    Write-Host "Executable size: $([math]::Round($fileSize / 1MB, 2)) MB" -ForegroundColor Cyan
    
    # Check for VCRUNTIME140.dll dependency
    Write-Host "Checking for VCRUNTIME140.dll dependency..." -ForegroundColor Yellow
    $dependencies = dumpbin /dependents $exePath 2>$null
    $vcRuntime = $dependencies | Select-String "VCRUNTIME140.dll"
    
    if ($vcRuntime) {
        Write-Host "❌ VCRUNTIME140.dll dependency found!" -ForegroundColor Red
        Write-Host $vcRuntime
    } else {
        Write-Host "✅ No VCRUNTIME140.dll dependency found" -ForegroundColor Green
    }
    
    # Check for MSVCRT references
    Write-Host "Checking for MSVCRT references..." -ForegroundColor Yellow
    $msvcrt = dumpbin /directives $exePath 2>$null | Select-String "MSVCRT"
    if ($msvcrt) {
        Write-Host "⚠️  MSVCRT references found:" -ForegroundColor Yellow
        Write-Host $msvcrt
    } else {
        Write-Host "✅ No MSVCRT references found" -ForegroundColor Green
    }
    
    # Test if executable runs
    Write-Host "Testing executable..." -ForegroundColor Yellow
    try {
        $process = Start-Process -FilePath $exePath -ArgumentList "--test" -PassThru -WindowStyle Hidden
        Start-Sleep -Seconds 2
        if ($process.HasExited) {
            Write-Host "✅ Executable runs without VCRUNTIME140.dll error" -ForegroundColor Green
        } else {
            Write-Host "✅ Executable started successfully" -ForegroundColor Green
            $process.Kill()
        }
    } catch {
        Write-Host "❌ Error running executable: $($_.Exception.Message)" -ForegroundColor Red
    }
    
} else {
    Write-Host "❌ Executable not found!" -ForegroundColor Red
}

Write-Host "Test completed." -ForegroundColor Green 