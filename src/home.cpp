#include "home.h"
#include "global_config.h"
#include "device_manager.h"
#include "no_device.h"
#include "apps_list.h"
#include "install_app.h"
#include "call_logs.h"
#include "contacts.h"
#include "messages.h"
#include "notifications.h"
#include "lifecycle.h"
#include "calendar.h"
#include "media_files.h"
#include "processes.h"
#include "widgets_list.h"
#include "keyboard_list.h"
#include "settings_screen.h"
#include "settings_helper.h"
#include "window_actions_helper.h"
#include "theme_popup.h"
#include "preference_popup.h"
#include "confirmation_dialog.h"
#include "adb_terminal.h"
#include "imgui.h"
#include <iostream>
#include <vector>
#include <string>

// Global variable to store current screen
static MenuScreen g_currentScreen = MenuScreen::APPS_LIST;

void SetCurrentScreen(MenuScreen screen) {
    g_currentScreen = screen;
}

MenuScreen GetCurrentScreen() {
    return g_currentScreen;
}

void ShowHome() {
    try {
        // Apply the current font
        ImFont* currentFont = GlobalConfig::GetCurrentFont();
        if (currentFont) {
            ImGui::PushFont(currentFont, currentFont->LegacySize);
        }
        
        // Get the window size
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        
        // Calculate menu bar height (typically around 20-25 pixels)
        float menuBarHeight = ImGui::GetFrameHeight();
        
        // Add main menu bar (before the window)
        if (ImGui::BeginMainMenuBar())  // Start top menu bar
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
            
            // Add Apps menu
            if (ImGui::BeginMenu("Apps"))
            {
                if (ImGui::MenuItem("Apps List")) {
                    SetCurrentScreen(MenuScreen::APPS_LIST);
                }
                if (ImGui::MenuItem("Install App")) {
                    SetCurrentScreen(MenuScreen::INSTALL_APP);
                }
                ImGui::EndMenu();
            }
            
            // Add Data menu
            if (ImGui::BeginMenu("Data"))
            {
                if (ImGui::MenuItem("Call Logs")) {
                    SetCurrentScreen(MenuScreen::CALL_LOGS);
                }
                if (ImGui::MenuItem("Contacts")) {
                    SetCurrentScreen(MenuScreen::CONTACTS);
                }
                if (ImGui::MenuItem("Messages")) {
                    SetCurrentScreen(MenuScreen::MESSAGES);
                }
                if (ImGui::MenuItem("Notifications")) {
                    SetCurrentScreen(MenuScreen::NOTIFICATIONS);
                }
                if (ImGui::MenuItem("Lifecycle")) {
                    SetCurrentScreen(MenuScreen::LIFECYCLE);
                }
                if (ImGui::MenuItem("Calendar")) {
                    SetCurrentScreen(MenuScreen::CALENDAR);
                }
                if (ImGui::MenuItem("Media Files")) {
                    SetCurrentScreen(MenuScreen::MEDIA_FILES);
                }
                if (ImGui::MenuItem("Processes")) {
                    SetCurrentScreen(MenuScreen::PROCESSES);
                }
                if (ImGui::MenuItem("Widgets List")) {
                    SetCurrentScreen(MenuScreen::WIDGETS_LIST);
                }
                if (ImGui::MenuItem("Keyboard List")) {
                    SetCurrentScreen(MenuScreen::KEYBOARD_LIST);
                }
                ImGui::EndMenu();
            }
            
            // Add Open menu 
            if (ImGui::BeginMenu("Open"))
            {
                if (ImGui::MenuItem("Setting screens")) {
                    SetCurrentScreen(MenuScreen::SETTINGS_SCREEN);
                }
                if (ImGui::MenuItem("ADB Terminal")) {
                    SetCurrentScreen(MenuScreen::ADB_TERMINAL);
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();  // End top menu bar
        }
        
        // Show theme popup if needed
        ShowThemePopup();
        
        // Show preference popup if needed
        ShowPreferencePopup();
        
        // Create a full-window, non-draggable window
        ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(windowSize.x, windowSize.y - menuBarHeight));
        
        // Window flags to make it non-draggable and full-window
        ImGuiWindowFlags windowFlags = 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;
        
        // Begin the window
        if (ImGui::Begin("Home", nullptr, windowFlags)) {
            // Get current devices (now handled in background by DeviceManager)
            std::vector<DeviceInfo> devices = GlobalConfig::GetDevices();
            
            // Show device tabs if devices are found (on the same line as ADBKing)
            if (!devices.empty()) {
                ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
                if (ImGui::BeginTabBar("DeviceTabs", tab_bar_flags)) {
                    for (const auto& device : devices) {
                        // Create tab name - only show device name if it's different from device ID
                        std::string tabName;
                        if (device.deviceName == device.deviceId || device.deviceName.empty()) {
                            // Device name is unknown or not found, show device ID
                            tabName = device.deviceId;
                        } else {
                            // Device name is available, show only the device name
                            tabName = device.deviceName;
                        }
                        
                        // Truncate if too long
                        if (tabName.length() > 20) {
                            tabName = tabName.substr(0, 17) + "...";
                        }
                        
                        if (ImGui::BeginTabItem(tabName.c_str())) {
                            // Store the selected device ID when tab is clicked
                            GlobalConfig::SetSelectedDeviceId(device.deviceId);
                            
                            ImGui::EndTabItem();
                        }
                    }
                    ImGui::EndTabBar();
                }
            }
            
            // Calculate available space for the main content area
            float availableHeight = (windowSize.y - menuBarHeight) - ImGui::GetCursorPosY() - 10.0f; // Leave some margin
            float availableWidth = windowSize.x - 20.0f; // Full width minus margins
            
            // Create empty ground for screen switching
            ImGui::BeginChild("ScreenArea", ImVec2(availableWidth, availableHeight), false);
            
            // Show no device screen when no devices are connected
            if (devices.empty()) {
                ShowNoDevice();
            } else {
                // Show the appropriate screen based on current selection
                switch (g_currentScreen) {
                    case MenuScreen::APPS_LIST:
                        ShowAppsList();
                        break;
                    case MenuScreen::INSTALL_APP:
                        ShowInstallApp();
                        break;
                    case MenuScreen::CALL_LOGS:
                        ShowCallLogs();
                        break;
                    case MenuScreen::CONTACTS:
                        ShowContacts();
                        break;
                    case MenuScreen::MESSAGES:
                        ShowMessages();
                        break;
                    case MenuScreen::NOTIFICATIONS:
                        ShowNotifications();
                        break;
                    case MenuScreen::LIFECYCLE:
                        ShowLifecycle();
                        break;
                    case MenuScreen::CALENDAR:
                        ShowCalendar();
                        break;
                    case MenuScreen::MEDIA_FILES:
                        ShowMediaFiles();
                        break;
                    case MenuScreen::PROCESSES:
                        ShowProcesses();
                        break;
                    case MenuScreen::WIDGETS_LIST:
                        ShowWidgetsList();
                        break;
                    case MenuScreen::KEYBOARD_LIST:
                        ShowKeyboardList();
                        break;
                    case MenuScreen::SETTINGS_SCREEN:
                        ShowSettingsScreen();
                        break;
                    case MenuScreen::ADB_TERMINAL:
                        ShowADBTerminal();
                        break;
                    default:
                        ShowAppsList(); // Default to Apps List
                        break;
                }
            }
            
            ImGui::EndChild();
        }
        ImGui::End();
        
        // Show confirmation dialogs (if any are open)
        ShowConfirmationDialog();
        
        // Pop the font if we pushed one
        if (GlobalConfig::GetCurrentFont()) {
            ImGui::PopFont();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in ShowHome: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in ShowHome" << std::endl;
    }
}


