#include "home.h"
#include "global_config.h"
#include "window_actions_helper.h"
#include "theme_popup.h"
#include "preference_popup.h"
#include "confirmation_dialog.h"
#include "spotlight.h"
#include "imgui.h"
#include <iostream>

static DevicePage g_currentPage = DevicePage::NONE;

void SetDevicePage(DevicePage page) {
    g_currentPage = page;
}

DevicePage GetDevicePage() {
    return g_currentPage;
}

static void ShowDevicePageContent(DevicePage page) {
    ImGui::BeginChild("DevicePageContent", ImVec2(0, 0), true);
    
    const char* title = "";
    switch (page) {
        case DevicePage::DEVICES_LIST:   title = "Devices List"; break;
        case DevicePage::EMULATORS:      title = "Emulators"; break;
        case DevicePage::WIRELESS_ADB:   title = "Wireless ADB"; break;
        case DevicePage::DEVELOPER_GUIDE: title = "Developer Guide"; break;
        default: break;
    }
    
    ImGui::TextWrapped("%s", title);
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("Coming soon...").x) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.5f);
    ImGui::Text("Coming soon...");
    ImGui::PopStyleColor();
    
    ImGui::EndChild();
}

void ShowHome() {
    try {
        
        // Add main menu bar (before the window)
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Devices"))
            {
                if (ImGui::MenuItem("Devices List")) {
                    SetDevicePage(DevicePage::DEVICES_LIST);
                }
                if (ImGui::MenuItem("Emulators")) {
                    SetDevicePage(DevicePage::EMULATORS);
                }
                if (ImGui::MenuItem("Wireless ADB")) {
                    SetDevicePage(DevicePage::WIRELESS_ADB);
                }
                if (ImGui::MenuItem("Developer Guide")) {
                    SetDevicePage(DevicePage::DEVELOPER_GUIDE);
                }
                ImGui::EndMenu();
            }
            
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

        {
            static bool registered = false;
            if (!registered) {
                RegisterSpotlightItem({"Devices List", "Devices", []() { SetDevicePage(DevicePage::DEVICES_LIST); }, nullptr});
                RegisterSpotlightItem({"Emulators", "Devices", []() { SetDevicePage(DevicePage::EMULATORS); }, nullptr});
                RegisterSpotlightItem({"Wireless ADB", "Devices", []() { SetDevicePage(DevicePage::WIRELESS_ADB); }, nullptr});
                RegisterSpotlightItem({"Developer Guide", "Devices", []() { SetDevicePage(DevicePage::DEVELOPER_GUIDE); }, nullptr});
                RegisterSpotlightItem({"Close", "Settings", []() { WindowActionsHelper::CloseMainWindow(); }, nullptr});
                RegisterSpotlightItem({"Theme", "Settings", []() { SetThemePopupVisible(true); }, []() { return IsThemePopupVisible(); }, {"dark", "light", "colors", "appearance", "style"}});
                RegisterSpotlightItem({"Preference", "Settings", []() { SetPreferencePopupVisible(true); }, []() { return IsPreferencePopupVisible(); }, {"settings", "options", "configure"}});
                registered = true;
            }
        }

        // Show popups if needed
        ShowThemePopup();
        ShowPreferencePopup();
        
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Home", nullptr)) {
            if (g_currentPage != DevicePage::NONE) {
                ShowDevicePageContent(g_currentPage);
            }
        }
        ImGui::End();
        
        ShowConfirmationDialog();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in ShowHome: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in ShowHome" << std::endl;
    }
}


