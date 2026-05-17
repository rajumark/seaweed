#pragma once

#include "imgui.h"
#include <string>
#include <memory>

class FontManager {
public:
    static FontManager& GetInstance();
    
    // Initialize the font manager
    void Initialize();
    
    // Load the default font (JetBrains Mono)
    ImFont* LoadDefaultFont(float sizePixels = 18.0f);
    
    // Load a specific font from file
    ImFont* LoadFontFromFile(const std::string& fontPath, float sizePixels = 18.0f);
    
    // Get the current default font
    ImFont* GetDefaultFont() const { return m_defaultFont; }
    
    // Check if JetBrains Mono font is loaded
    bool IsJetBrainsMonoLoaded() const { return m_defaultFont != nullptr; }
    
    // Get the fallback font (ProggyClean)
    ImFont* GetFallbackFont() const;
    
    // Set font size and reload if necessary
    void SetFontSize(float sizePixels);
    
    // Get current font size
    float GetFontSize() const { return m_currentFontSize; }
    
    // Check if fonts need to be reloaded
    bool FontsNeedReload() const { return m_fontsNeedReload; }
    
    // Reload fonts with current settings
    void ReloadFonts();
    
    // Mark fonts as reloaded
    void SetFontsReloaded() { m_fontsNeedReload = false; }
    
    // Cleanup
    void Cleanup();

private:
    FontManager() = default;
    ~FontManager() = default;
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    
    ImFont* m_defaultFont = nullptr;
    float m_currentFontSize = 18.0f;
    bool m_initialized = false;
    bool m_fontsNeedReload = false; // New flag
    
    // Try to load JetBrains Mono font
    ImFont* TryLoadJetBrainsMono(float sizePixels);
    
    // Try to load embedded JetBrains Mono font
    ImFont* TryLoadEmbeddedJetBrainsMono(float sizePixels);
    
    // Fallback to default ImGui font
    ImFont* LoadFallbackFont(float sizePixels);
};
