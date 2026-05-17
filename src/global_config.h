#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "device_manager.h"

// Forward declaration
struct ImFont;

// Theme enumeration
enum class Theme {
    LIGHT,
    DARK
};

// Global ADB path configuration
namespace GlobalConfig {
    extern std::string g_adbPath;
    extern Theme g_currentTheme;
    extern int g_fontSize;
    extern ImFont* g_currentFont;
    
    // Function to set the ADB path globally
    void SetADBPath(const std::string& path);
    
    // Function to get the ADB path
    std::string GetADBPath();
    
    // Function to initialize the ADB path (called during setup)
    void InitializeADBPath();
    
    // Theme management functions
    void SetTheme(Theme theme);
    Theme GetCurrentTheme();
    void ApplyTheme(Theme theme);
    
    // Font size management functions
    void SetFontSize(int fontSize);
    int GetFontSize();
    void ApplyFontSize(int fontSize);
    ImFont* GetCurrentFont();
    void LoadFont(int fontSize);
    bool FontNeedsReload();
    void SetFontReloaded();
    
    // Device management functions (now delegate to DeviceManager)
    void UpdateDeviceList();
    std::vector<DeviceInfo> GetDevices();
    std::string GetSelectedDeviceId();
    void SetSelectedDeviceId(const std::string& deviceId);
    bool IsDeviceConnected(const std::string& deviceId);
    void ClearDevices();
    
    // Initialize device manager
    void InitializeDeviceManager();
    
    // Start/stop device monitoring
    void StartDeviceMonitoring();
    void StopDeviceMonitoring();
}
