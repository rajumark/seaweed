#include "preference_popup.h"
#include "global_config.h"
#include "imgui.h"
#include "key_value_store.h"
#include <string>

// Global variable to store preference popup state
static bool g_showPreferencePopup = false;

void ShowPreferencePopup() {
    if (!g_showPreferencePopup) return;
    
    // Center the popup on screen
    ImVec2 center = ImGui::GetIO().DisplaySize;
    center.x *= 0.5f;
    center.y *= 0.5f;
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_Appearing);
    
    ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoResize | 
                                 ImGuiWindowFlags_NoCollapse | 
                                 ImGuiWindowFlags_Modal;
    
    if (ImGui::Begin("Preferences", &g_showPreferencePopup, popupFlags)) {
        ImGuiIO& io = ImGui::GetIO();

        // Font size section (load at target size instead of scaling)
        ImGui::Text("Font Size");
        ImGui::Separator();

        static int fontPx = 18;
        if (ImGui::IsWindowAppearing()) {
            fontPx = GlobalConfig::GetFontSize();
        }

        if (ImGui::SliderInt("Size (px)", &fontPx, 10, 32)) {
            GlobalConfig::SetFontSize(fontPx);
            KeyValueStore::SetInt("font_px", fontPx);
        }

        ImGui::Text("Hint: Keep GlobalScale at 1.0 for sharpest text");
        ImGui::Spacing();
        ImGui::Separator();

        // UI Style Controls
        ImGui::Text("UI Style");
        ImGui::Separator();
        ImGuiStyle& style = ImGui::GetStyle();
        float frameRounding = style.FrameRounding;
        if (ImGui::SliderFloat("FrameRounding", &frameRounding, 0.0f, 12.0f, "%.0f")) {
            style.FrameRounding = frameRounding;
            style.GrabRounding = frameRounding;
            KeyValueStore::SetFloat("style_frame_rounding", frameRounding);
        }
        bool windowBorder = (style.WindowBorderSize > 0.0f);
        if (ImGui::Checkbox("WindowBorder", &windowBorder)) {
            style.WindowBorderSize = windowBorder ? 1.0f : 0.0f;
            KeyValueStore::SetFloat("style_window_border", style.WindowBorderSize);
        }
        ImGui::SameLine();
        bool frameBorder = (style.FrameBorderSize > 0.0f);
        if (ImGui::Checkbox("FrameBorder", &frameBorder)) {
            style.FrameBorderSize = frameBorder ? 1.0f : 0.0f;
            KeyValueStore::SetFloat("style_frame_border", style.FrameBorderSize);
        }
        ImGui::SameLine();
        bool popupBorder = (style.PopupBorderSize > 0.0f);
        if (ImGui::Checkbox("PopupBorder", &popupBorder)) {
            style.PopupBorderSize = popupBorder ? 1.0f : 0.0f;
            KeyValueStore::SetFloat("style_popup_border", style.PopupBorderSize);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Confirmation Dialogs Section
        ImGui::Text("Confirmation Dialogs");
        ImGui::Separator();
        
        // Load current confirmation dialog settings
        static bool showUninstallConfirm = true;
        static bool showClearDataConfirm = true;
        
        if (ImGui::IsWindowAppearing()) {
            showUninstallConfirm = !KeyValueStore::GetBool("confirm_uninstall_disabled", false);
            showClearDataConfirm = !KeyValueStore::GetBool("confirm_clear_data_disabled", false);
        }
        
        // Uninstall confirmation checkbox
        if (ImGui::Checkbox("Show uninstall confirmation", &showUninstallConfirm)) {
            KeyValueStore::SetBool("confirm_uninstall_disabled", !showUninstallConfirm);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextWrapped("When enabled, shows a confirmation dialog before uninstalling apps. You can also disable this permanently from the confirmation dialog itself.");
            ImGui::EndTooltip();
        }
        
        // Clear data confirmation checkbox
        if (ImGui::Checkbox("Show clear data confirmation", &showClearDataConfirm)) {
            KeyValueStore::SetBool("confirm_clear_data_disabled", !showClearDataConfirm);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextWrapped("When enabled, shows a confirmation dialog before clearing app data. You can also disable this permanently from the confirmation dialog itself.");
            ImGui::EndTooltip();
        }
        
        ImGui::Spacing();
        
        // Reset confirmation dialog settings to defaults
        if (ImGui::Button("Reset to Defaults", ImVec2(150, 0))) {
            showUninstallConfirm = true;
            showClearDataConfirm = true;
            KeyValueStore::SetBool("confirm_uninstall_disabled", false);
            KeyValueStore::SetBool("confirm_clear_data_disabled", false);
        }
        
        ImGui::Spacing();
        
        // Close button
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            g_showPreferencePopup = false;
        }
    }
    ImGui::End();
}

// Function to set the preference popup visibility (needed by home.cpp)
void SetPreferencePopupVisible(bool visible) {
    g_showPreferencePopup = visible;
}
