#include "global_config.h"
#include "key_value_store.h"
#include "setup.h"
#include "device_manager.h"
#include "font_manager.h"
#include "imgui.h"
#include <iostream>
#include <filesystem>

namespace GlobalConfig {
    std::string g_adbPath;
    Theme g_currentTheme = Theme::DARK; // Default to dark theme
    int g_fontSize = 18; // Default font size (for sharper readability)
    ImFont* g_currentFont = nullptr; // Current font pointer
    bool g_fontNeedsReload = false; // Flag to indicate if font needs to be reloaded
    
    void SetADBPath(const std::string& path) {
        g_adbPath = path;
    }
    
    std::string GetADBPath() {
        return g_adbPath;
    }
    
    void InitializeADBPath() {
        // Use dynamic path from setup.cpp
        std::string adbKingPath = GetADBKingPath();
        
        // Use platform-specific path separators
#ifdef _WIN32
        std::string platformToolsPath = adbKingPath + "\\platform-tools";
        std::string adbExecutable = platformToolsPath + "\\adb.exe";
#else
        std::string platformToolsPath = adbKingPath + "/platform-tools";
        std::string adbExecutable = platformToolsPath + "/adb";
#endif
        
        // Check if the ADB executable exists
        if (std::filesystem::exists(adbExecutable)) {
            g_adbPath = adbExecutable;
        } else {
            // Fallback to empty string if ADB doesn't exist
            g_adbPath = "";
        }
    }
    
    void SetTheme(Theme theme) {
        g_currentTheme = theme;
        ApplyTheme(theme);
        // Persist theme preference
        KeyValueStore::SetString("theme", theme == Theme::DARK ? "dark" : "light");
    }
    
    Theme GetCurrentTheme() {
        return g_currentTheme;
    }
    
    void ApplyTheme(Theme theme) {
        if (theme == Theme::DARK) {
            ImGui::StyleColorsDark();
        } else {
            ImGui::StyleColorsLight();
        }
    }
    
    void SetFontSize(int fontSize) {
        if (g_fontSize != fontSize) {
            g_fontSize = fontSize;
            g_fontNeedsReload = true;
            
            // Don't immediately reload fonts here - it can cause crashes during ImGui rendering
            // The fonts will be reloaded when LoadFont is called at a safe time
        }
    }
    
    int GetFontSize() {
        return g_fontSize;
    }
    
    void ApplyFontSize(int fontSize) {
        SetFontSize(fontSize);
    }
    
    ImFont* GetCurrentFont() {
        if (g_currentFont == nullptr) {
            g_currentFont = FontManager::GetInstance().GetDefaultFont();
        }
        return g_currentFont;
    }
    
    bool FontNeedsReload() {
        return g_fontNeedsReload;
    }
    
    void SetFontReloaded() {
        g_fontNeedsReload = false;
    }
    
    void LoadFont(int fontSize) {
        // Use the FontManager to load the default font (JetBrains Mono)
        g_currentFont = FontManager::GetInstance().LoadDefaultFont(static_cast<float>(fontSize));
        
        // Mark that fonts have been reloaded
        g_fontNeedsReload = false;
    }
    
    void InitializeDeviceManager() {
        DeviceManager::GetInstance().Initialize(g_adbPath);
    }
    
    void StartDeviceMonitoring() {
        DeviceManager::GetInstance().StartMonitoring();
    }
    
    void StopDeviceMonitoring() {
        DeviceManager::GetInstance().StopMonitoring();
    }
    
    void UpdateDeviceList() {
        // This function is now deprecated - device list is updated automatically in background
        // Keeping for backward compatibility but it does nothing
    }
    
    std::vector<DeviceInfo> GetDevices() {
        return DeviceManager::GetInstance().GetDevices();
    }
    
    std::string GetSelectedDeviceId() {
        return DeviceManager::GetInstance().GetSelectedDeviceId();
    }
    
    void SetSelectedDeviceId(const std::string& deviceId) {
        DeviceManager::GetInstance().SetSelectedDeviceId(deviceId);
    }
    
    bool IsDeviceConnected(const std::string& deviceId) {
        return DeviceManager::GetInstance().IsDeviceConnected(deviceId);
    }
    
    void ClearDevices() {
        DeviceManager::GetInstance().ClearDevices();
    }
}
