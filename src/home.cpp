#include "home.h"
#include "global_config.h"
#include "window_actions_helper.h"
#include "theme_popup.h"
#include "preference_popup.h"
#include "confirmation_dialog.h"
#include "imgui.h"
#include <iostream>

void ShowHome() {
    try {
        
        // Get the window size
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        
        // Calculate menu bar height (typically around 20-25 pixels)
        float menuBarHeight = ImGui::GetFrameHeight();
        
        // Add main menu bar (before the window)
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Settings"))
            {
                if (ImGui::MenuItem("Close")) {
                    WindowActionsHelper::CloseMainWindow();
                }
                if (ImGui::MenuItem("Theme")) {
                    SetThemePopupVisible(true);
                }
                if (ImGui::MenuItem("Preference")) {
                    SetPreferencePopupVisible(true);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        
        // Show popups if needed
        ShowThemePopup();
        ShowPreferencePopup();
        
        // Create a full-window, non-draggable window
        ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(windowSize.x, windowSize.y - menuBarHeight));
        
        ImGuiWindowFlags windowFlags = 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;
        
        if (ImGui::Begin("Home", nullptr, windowFlags)) {
            // Empty main area — nothing rendered yet
        }
        ImGui::End();
        
        ShowConfirmationDialog();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in ShowHome: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in ShowHome" << std::endl;
    }
}


