#include "settings_screen.h"
#include "settings_helper.h"
#include "global_config.h"
#include "settings_pinned_store.h"
#include "settings_action_helper.h"
#include "imgui.h"
#include <string>
#include <unordered_set>

static char g_searchBuffer[256] = ""; // Search input buffer
static std::string g_selectedSettingId = ""; // Track which setting is selected for context menu

void ShowSettingsScreen() {
    ImGui::Text("Settings List");
    ImGui::Separator();
    
    // Get current device ID
    std::string deviceId = GlobalConfig::GetSelectedDeviceId();
    
    // Show device info
    if (deviceId.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No device selected");
    }
    
    ImGui::Spacing();
    
    // Search input field
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputTextWithHint("##SearchSettings", "Search settings", g_searchBuffer, sizeof(g_searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Optional: Add any immediate search behavior here if needed
    }
    
    ImGui::Spacing();
    
    // Show error if no device
    if (deviceId.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Please select a device from the device tabs above to use settings");
        return;
    }
    
    // Get filtered settings buttons
    std::vector<SettingsButton> filteredButtons = SettingsHelper::FilterSettingsButtons(std::string(g_searchBuffer));
    
    // Get pinned settings
    std::vector<std::string> pinnedSettings = SettingsPinnedStore::GetPinnedSettings();
    size_t pinnedCount = SettingsPinnedStore::GetPinnedCount();
    
    // Show total count with pinned count
    ImGui::Text("Total: %d (Filtered: %d, Pinned: %d)", 
                 (int)SettingsHelper::GetSettingsButtons().size(), 
                 (int)filteredButtons.size(), 
                 (int)pinnedCount);
    
    ImGui::Spacing();
    
    // Display settings buttons in a grid-like layout
    ImGui::BeginChild("SettingsButtons", ImVec2(0, 0), false);
    
    float buttonWidth = 200.0f;
    float buttonHeight = 30.0f;
    float spacing = 10.0f;
    
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    float availableWidth = windowSize.x - 20.0f; // Leave some margin
    
    int buttonsPerRow = static_cast<int>(availableWidth / (buttonWidth + spacing));
    if (buttonsPerRow < 1) buttonsPerRow = 1;
    
    // Create a fast lookup set for pinned settings
    std::unordered_set<std::string> pinnedSet(pinnedSettings.begin(), pinnedSettings.end());
    
    // Filter pinned settings based on search
    std::vector<std::string> filteredPinnedSettings;
    if (!pinnedSettings.empty()) {
        filteredPinnedSettings.reserve(pinnedSettings.size());
        for (const auto& settingId : pinnedSettings) {
            // Find the corresponding SettingsButton to check if it matches search
            for (const auto& button : filteredButtons) {
                if (button.id == settingId) {
                    filteredPinnedSettings.push_back(settingId);
                    break;
                }
            }
        }
    }
    
    // Show pinned settings section if there are any pinned settings
    if (!filteredPinnedSettings.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.8f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.9f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));
        
        if (ImGui::CollapsingHeader("Pinned Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& settingId : filteredPinnedSettings) {
                // Find the corresponding SettingsButton
                const SettingsButton* button = nullptr;
                for (const auto& btn : filteredButtons) {
                    if (btn.id == settingId) {
                        button = &btn;
                        break;
                    }
                }
                
                if (!button) continue; // Skip if button not found
                
                bool isSelected = (g_selectedSettingId == settingId);
                
                // Create button with unique ID
                std::string buttonId = "##Pinned_" + button->id;
                
                if (ImGui::Button(button->text.c_str(), ImVec2(buttonWidth, buttonHeight))) {
                    // Execute the settings intent
                    if (SettingsHelper::ExecuteSettingsIntent(deviceId, button->id, button->intent)) {
                        // Success - could show a success message here
                    } else {
                        // Failed - could show an error message here
                    }
                }
                
                // Right-click context menu for pinned settings
                if (ImGui::BeginPopupContextItem(("##ContextMenu_" + settingId).c_str())) {
                    g_selectedSettingId = settingId;
                    
                    if (ImGui::MenuItem("Execute")) {
                        if (SettingsHelper::ExecuteSettingsIntent(deviceId, button->id, button->intent)) {
                            // Success
                        } else {
                            // Failed
                        }
                    }

                    if (ImGui::MenuItem("Unpin")) {
                        SettingsActionHelper::UnpinSetting(settingId);
                    }
                    ImGui::EndPopup();
                }
                
                // Tooltip showing the intent
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Intent: %s", button->intent.c_str());
                    ImGui::Text("ID: %s", button->id.c_str());
                    ImGui::Text("Status: Pinned");
                    ImGui::EndTooltip();
                }
                
                // Move to next row if needed
                if ((filteredPinnedSettings.size() > 1) && 
                    (&settingId - &filteredPinnedSettings[0] + 1) % buttonsPerRow != 0) {
                    ImGui::SameLine();
                }
            }
        }
        
        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }
    
    // Show all settings section
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    if (ImGui::CollapsingHeader("All Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (size_t i = 0; i < filteredButtons.size(); ++i) {
            const auto& button = filteredButtons[i];
            
            // Skip if this setting is already shown in pinned section
            if (pinnedSet.find(button.id) != pinnedSet.end()) {
                continue;
            }
            
            if (i > 0 && i % buttonsPerRow != 0) {
                ImGui::SameLine();
            }
            
            bool isSelected = (g_selectedSettingId == button.id);
            
            // Create button with unique ID
            std::string buttonId = "##" + button.id;
            
            if (ImGui::Button(button.text.c_str(), ImVec2(buttonWidth, buttonHeight))) {
                // Execute the settings intent
                if (SettingsHelper::ExecuteSettingsIntent(deviceId, button.id, button.intent)) {
                    // Success - could show a success message here
                } else {
                    // Failed - could show an error message here
                }
            }
            
            // Right-click context menu for all settings
            if (ImGui::BeginPopupContextItem(("##ContextMenu_" + button.id).c_str())) {
                g_selectedSettingId = button.id;
                
                if (ImGui::MenuItem("Execute")) {
                    if (SettingsHelper::ExecuteSettingsIntent(deviceId, button.id, button.intent)) {
                        // Success
                    } else {
                        // Failed
                    }
                }

                if (ImGui::MenuItem("Pin")) {
                    SettingsActionHelper::PinSetting(button.id);
                }
                ImGui::EndPopup();
            }
            
            // Tooltip showing the intent
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Intent: %s", button.intent.c_str());
                ImGui::Text("ID: %s", button.id.c_str());
                ImGui::EndTooltip();
            }
        }
    }
    
    ImGui::PopStyleColor(3);
    
    // Clear selection when no context menu is open
    if (!g_selectedSettingId.empty() && !ImGui::IsPopupOpen(("##ContextMenu_" + g_selectedSettingId).c_str())) {
        g_selectedSettingId = "";
    }
    
    ImGui::EndChild();
}
