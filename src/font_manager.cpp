#include "font_manager.h"
#include "embedded_fonts.h"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

// Helper function to get the executable's actual path
std::string GetExecutablePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
#else
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return std::string(path);
    }
    return "";
#endif
}

// Helper function to find font file relative to executable
std::string FindFontFileRelativeToExecutable(const std::string& fontName) {
    try {
        // Get both current working directory and executable path
        std::filesystem::path exePath = std::filesystem::current_path();
        std::string actualExePath = GetExecutablePath();
        std::filesystem::path exeDir;
        
        if (!actualExePath.empty()) {
            exeDir = std::filesystem::path(actualExePath).parent_path();
        }
        
        // Try different relative paths from both locations
        std::vector<std::filesystem::path> searchPaths = {
            exePath / fontName,
            exePath / "imgui" / "misc" / "fonts" / fontName,
            exePath / "misc" / "fonts" / fontName,
            exePath / "fonts" / fontName,
            exePath / ".." / "imgui" / "misc" / "fonts" / fontName,
            exePath / ".." / ".." / "imgui" / "misc" / "fonts" / fontName,
            exePath / ".." / ".." / ".." / "imgui" / "misc" / "fonts" / fontName
        };
        
        // Also try paths relative to executable directory
        if (!exeDir.empty()) {
            searchPaths.push_back(exeDir / fontName);
            searchPaths.push_back(exeDir / "imgui" / "misc" / "fonts" / fontName);
            searchPaths.push_back(exeDir / "misc" / "fonts" / fontName);
            searchPaths.push_back(exeDir / "fonts" / fontName);
        }
        
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                return path.string();
            }
        }
        
        return "";
    } catch (const std::exception& e) {
        return "";
    }
}

FontManager& FontManager::GetInstance() {
    static FontManager instance;
    return instance;
}

void FontManager::Initialize() {
    if (m_initialized) {
        return;
    }
    
    m_initialized = true;
}

ImFont* FontManager::LoadDefaultFont(float sizePixels) {
    if (!m_initialized) {
        Initialize();
    }
    
    // Clear existing fonts
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    
    // Try to load JetBrains Mono first from file
    m_defaultFont = TryLoadJetBrainsMono(sizePixels);
    
    if (m_defaultFont == nullptr) {
        // Try to load embedded JetBrains Mono font as fallback
        m_defaultFont = TryLoadEmbeddedJetBrainsMono(sizePixels);
        
        if (m_defaultFont == nullptr) {
            // Final fallback to default ImGui font
            m_defaultFont = LoadFallbackFont(sizePixels);
        }
    }
    
    m_currentFontSize = sizePixels;
    return m_defaultFont;
}

ImFont* FontManager::TryLoadJetBrainsMono(float sizePixels) {
    // Use the helper function to find the font file
    std::string fontPath = FindFontFileRelativeToExecutable("JetBrainsMono.ttf");
    
    if (!fontPath.empty()) {
        ImGuiIO& io = ImGui::GetIO();
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), sizePixels);
        if (font != nullptr) {
            return font;
        }
    }
    
    return nullptr;
}

ImFont* FontManager::TryLoadEmbeddedJetBrainsMono(float sizePixels) {
    // Try to load embedded font data if available
    if (EmbeddedFonts::IsEmbeddedFontAvailable()) {
        const unsigned char* fontData = EmbeddedFonts::GetJetBrainsMonoData();
        int fontDataSize = EmbeddedFonts::GetJetBrainsMonoDataSize();
        
        if (fontData && fontDataSize > 0) {
            ImGuiIO& io = ImGui::GetIO();
            ImFont* font = io.Fonts->AddFontFromMemoryTTF((void*)fontData, fontDataSize, sizePixels);
            if (font != nullptr) {
                return font;
            }
        }
    }
    
    return nullptr;
}

ImFont* FontManager::LoadFallbackFont(float sizePixels) {
    ImGuiIO& io = ImGui::GetIO();
    
    ImFontConfig fontConfig;
    fontConfig.SizePixels = sizePixels;
    return io.Fonts->AddFontDefault(&fontConfig);
}

ImFont* FontManager::LoadFontFromFile(const std::string& fontPath, float sizePixels) {
    if (!std::filesystem::exists(fontPath)) {
        return nullptr;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    return io.Fonts->AddFontFromFileTTF(fontPath.c_str(), sizePixels);
}

ImFont* FontManager::GetFallbackFont() const {
    // This function should return the fallback font, but since we can't access it directly,
    // we'll return nullptr and handle the comparison differently
    return nullptr;
}

void FontManager::SetFontSize(float sizePixels) {
    if (m_currentFontSize != sizePixels) {
        m_currentFontSize = sizePixels;
        m_fontsNeedReload = true;
        // Don't immediately reload fonts - it will be done when ReloadFonts is called at a safe time
    }
}

void FontManager::ReloadFonts() {
    LoadDefaultFont(m_currentFontSize);
    m_fontsNeedReload = false;
}

void FontManager::Cleanup() {
    m_defaultFont = nullptr;
    m_initialized = false;
}
