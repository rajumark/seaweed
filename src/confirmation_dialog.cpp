#include "confirmation_dialog.h"
#include "imgui.h"
#include "key_value_store.h"
#include <sstream>

// Global confirmation dialog state
static ConfirmationDialogState g_confirmationState;

void ShowConfirmationDialog() {
    if (!g_confirmationState.isOpen) return;
    
    // Center the popup on screen
    ImVec2 center = ImGui::GetIO().DisplaySize;
    center.x *= 0.5f;
    center.y *= 0.5f;
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Appearing);
    
    ImGuiWindowFlags popupFlags = ImGuiWindowFlags_NoResize | 
                                 ImGuiWindowFlags_NoCollapse | 
                                 ImGuiWindowFlags_Modal;
    
    if (ImGui::Begin(g_confirmationState.title.c_str(), &g_confirmationState.isOpen, popupFlags)) {
        // Show the confirmation message
        ImGui::TextWrapped("%s", g_confirmationState.message.c_str());
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // "Do not show again" checkbox
        ImGui::Checkbox("Do not show this dialog again", &g_confirmationState.doNotShowAgain);
        
        ImGui::Spacing();
        
        // Buttons
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        
        // Cancel button (left-aligned)
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            if (g_confirmationState.onCancel) {
                g_confirmationState.onCancel();
            }
            g_confirmationState.isOpen = false;
        }
        
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 180);
        
        // Confirm button (right-aligned)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
        
        if (ImGui::Button("Confirm", ImVec2(80, 0))) {
            // Save "do not show again" preference if checked
            if (g_confirmationState.doNotShowAgain) {
                std::string key = (g_confirmationState.type == ConfirmationType::Uninstall) 
                    ? "confirm_uninstall_disabled" 
                    : "confirm_clear_data_disabled";
                KeyValueStore::SetBool(key, true);
            }
            
            // Execute the confirmation callback
            if (g_confirmationState.onConfirm) {
                g_confirmationState.onConfirm();
            }
            
            g_confirmationState.isOpen = false;
        }
        
        ImGui::PopStyleColor(3);
    }
    ImGui::End();
}

void OpenConfirmationDialog(ConfirmationType type, const std::string& packageName, 
                           std::function<void()> onConfirm, std::function<void()> onCancel) {
    g_confirmationState.isOpen = true;
    g_confirmationState.type = type;
    g_confirmationState.packageName = packageName;
    g_confirmationState.doNotShowAgain = false;
    g_confirmationState.onConfirm = onConfirm;
    g_confirmationState.onCancel = onCancel;
    
    // Set title and message based on type
    if (type == ConfirmationType::Uninstall) {
        g_confirmationState.title = "Confirm Uninstall";
        std::ostringstream oss;
        oss << "Are you sure you want to uninstall \"" << packageName << "\"?\n\n";
        oss << "This action cannot be undone. The app will be completely removed from the device.";
        g_confirmationState.message = oss.str();
    } else if (type == ConfirmationType::ClearData) {
        g_confirmationState.title = "Confirm Clear Data";
        std::ostringstream oss;
        oss << "Are you sure you want to clear all data for \"" << packageName << "\"?\n\n";
        oss << "This will delete all app data, settings, and cache. The app will be reset to its initial state.";
        g_confirmationState.message = oss.str();
    }
}

bool ShouldShowConfirmation(ConfirmationType type) {
    std::string key = (type == ConfirmationType::Uninstall) 
        ? "confirm_uninstall_disabled" 
        : "confirm_clear_data_disabled";
    
    return !KeyValueStore::GetBool(key, false);
}
