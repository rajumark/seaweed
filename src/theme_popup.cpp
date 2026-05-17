#include "theme_popup.h"
#include "global_config.h"
#include "imgui.h"
#include "key_value_store.h"

// Global variable to store theme popup state
static bool g_showThemePopup = false;

void ShowThemePopup() {
    if (!g_showThemePopup) return;
    
    // Center the popup on screen
    ImVec2 center = ImGui::GetIO().DisplaySize;
    center.x *= 0.5f;
    center.y *= 0.5f;
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(180, 100), ImGuiCond_Appearing);
    
    ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoResize | 
                                 ImGuiWindowFlags_NoCollapse | 
                                 ImGuiWindowFlags_Modal;
    
    if (ImGui::Begin("Theme Selection", &g_showThemePopup, popupFlags)) {
        // Get current theme
        Theme currentTheme = GlobalConfig::GetCurrentTheme();
        
        // Light theme option
        bool lightSelected = (currentTheme == Theme::LIGHT);
        if (ImGui::RadioButton("Light Theme", lightSelected)) {
            if (!lightSelected) {
                GlobalConfig::SetTheme(Theme::LIGHT);
                KeyValueStore::SetString("theme", "light");
            }
        }
        
        ImGui::Spacing();
        
        // Dark theme option
        bool darkSelected = (currentTheme == Theme::DARK);
        if (ImGui::RadioButton("Dark Theme", darkSelected)) {
            if (!darkSelected) {
                GlobalConfig::SetTheme(Theme::DARK);
                KeyValueStore::SetString("theme", "dark");
            }
        }
    }
    ImGui::End();
}

// Function to set the theme popup visibility (needed by home.cpp)
void SetThemePopupVisible(bool visible) {
    g_showThemePopup = visible;
}

bool IsThemePopupVisible() {
    return g_showThemePopup;
}
