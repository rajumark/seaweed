#include "settings_panel.h"
#include "core/registry/panel_registry.h"
#include "global_config.h"
#include "device_manager.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <algorithm>
#include <thread>

struct SettingItem {
    std::string id;
    std::string name;
    std::string intent;
};

static std::vector<SettingItem> MakeSettingsList() {
    return {
        {"settings", "Settings", "android.settings.SETTINGS"},
        {"apn_settings", "APN Settings", "android.settings.APN_SETTINGS"},
        {"location_source_settings", "Location Source Settings", "android.settings.LOCATION_SOURCE_SETTINGS"},
        {"location_scanning_settings", "Location Scanning Settings", "android.settings.LOCATION_SCANNING_SETTINGS"},
        {"user_settings", "User Settings", "android.settings.USER_SETTINGS"},
        {"wireless_settings", "Wireless Settings", "android.settings.WIRELESS_SETTINGS"},
        {"airplane_mode_settings", "Airplane Mode Settings", "android.settings.AIRPLANE_MODE_SETTINGS"},
        {"mobile_data_usage", "Mobile Data Usage", "android.settings.MOBILE_DATA_USAGE"},
        {"one_handed_settings", "One-Handed Settings", "android.settings.action.ONE_HANDED_SETTINGS"},
        {"accessibility_settings", "Accessibility Settings", "android.settings.ACCESSIBILITY_SETTINGS"},
        {"color_inversion_settings", "Color Inversion Settings", "android.settings.COLOR_INVERSION_SETTINGS"},
        {"security_settings", "Security Settings", "android.settings.SECURITY_SETTINGS"},
        {"privacy_settings", "Privacy Settings", "android.settings.PRIVACY_SETTINGS"},
        {"vpn_settings", "VPN Settings", "android.settings.VPN_SETTINGS"},
        {"wifi_settings", "WiFi Settings", "android.settings.WIFI_SETTINGS"},
        {"data_usage_settings", "Data Usage Settings", "android.settings.DATA_USAGE_SETTINGS"},
        {"bluetooth_settings", "Bluetooth Settings", "android.settings.BLUETOOTH_SETTINGS"},
        {"cast_settings", "Cast Settings", "android.settings.CAST_SETTINGS"},
        {"date_settings", "Date Settings", "android.settings.DATE_SETTINGS"},
        {"sound_settings", "Sound Settings", "android.settings.SOUND_SETTINGS"},
        {"display_settings", "Display Settings", "android.settings.DISPLAY_SETTINGS"},
        {"auto_rotate_settings", "Auto Rotate Settings", "android.settings.AUTO_ROTATE_SETTINGS"},
        {"night_display_settings", "Night Display Settings", "android.settings.NIGHT_DISPLAY_SETTINGS"},
        {"dark_theme_settings", "Dark Theme Settings", "android.settings.DARK_THEME_SETTINGS"},
        {"locale_settings", "Locale Settings", "android.settings.LOCALE_SETTINGS"},
        {"lock_screen_settings", "Lock Screen Settings", "android.settings.LOCK_SCREEN_SETTINGS"},
        {"input_method_settings", "Input Method Settings", "android.settings.INPUT_METHOD_SETTINGS"},
        {"application_settings", "Application Settings", "android.settings.APPLICATION_SETTINGS"},
        {"application_development_settings", "Application Development Settings", "android.settings.APPLICATION_DEVELOPMENT_SETTINGS"},
        {"manage_applications_settings", "Manage Applications Settings", "android.settings.MANAGE_APPLICATIONS_SETTINGS"},
        {"system_update_settings", "System Update Settings", "android.settings.SYSTEM_UPDATE_SETTINGS"},
        {"sync_settings", "Sync Settings", "android.settings.SYNC_SETTINGS"},
        {"storage_settings", "Internal Storage Settings", "android.settings.INTERNAL_STORAGE_SETTINGS"},
        {"nfc_settings", "NFC Settings", "android.settings.NFC_SETTINGS"},
        {"dream_settings", "Dream Settings", "android.settings.DREAM_SETTINGS"},
        {"notification_settings", "Notification Settings", "android.settings.NOTIFICATION_SETTINGS"},
        {"battery_saver_settings", "Battery Saver Settings", "android.settings.BATTERY_SAVER_SETTINGS"},
        {"home_settings", "Home Settings", "android.settings.HOME_SETTINGS"},
        {"manage_default_apps_settings", "Manage Default Apps Settings", "android.settings.MANAGE_DEFAULT_APPS_SETTINGS"},
        {"print_settings", "Print Settings", "android.settings.ACTION_PRINT_SETTINGS"},
        {"zen_mode_settings", "Zen Mode Settings", "android.settings.ZEN_MODE_SETTINGS"},
        {"device_info_settings", "Device Info Settings", "android.settings.DEVICE_INFO_SETTINGS"},
        {"tether_settings", "Tether Settings", "android.settings.TETHER_SETTINGS"},
        {"wifi_tether_settings", "WiFi Tether Settings", "com.android.settings.WIFI_TETHER_SETTINGS"},
        {"bedtime_settings", "Bedtime Settings", "android.settings.BEDTIME_SETTINGS"},
    };
}

static void OpenSettingAsync(const std::string& intent) {
    std::string deviceId = GlobalConfig::GetSelectedDeviceId();
    if (deviceId.empty()) return;

    std::thread([intent, deviceId]() {
        std::string cmd = GlobalConfig::BuildAdbCommand("-s " + deviceId + " shell am start -a " + intent);
        DeviceManager::GetInstance().ExecuteCommand(cmd);
    }).detach();
}

static bool s_show = false;

static void DrawSettingsPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Settings", &s_show)) {
        static std::vector<SettingItem> items = MakeSettingsList();
        static char filterBuf[256] = {};
        static int selectedIndex = -1;

        ImGui::Text("Total: %zu", items.size());
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##filter", "Search settings...", filterBuf, sizeof(filterBuf));

        std::string filter = filterBuf;
        std::transform(filter.begin(), filter.end(), filter.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        ImGui::Separator();
        ImGui::BeginChild("SettingsList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (size_t i = 0; i < items.size(); i++) {
            std::string haystack = items[i].name;
            std::transform(haystack.begin(), haystack.end(), haystack.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (!filter.empty() && haystack.find(filter) == std::string::npos)
                continue;

            bool isSelected = (selectedIndex == static_cast<int>(i));
            ImGui::PushID(static_cast<int>(i));

            if (ImGui::Selectable(items[i].name.c_str(), &isSelected)) {
                selectedIndex = static_cast<int>(i);
                OpenSettingAsync(items[i].intent);
            }

            ImGui::PopID();
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void RegisterSettingsPanel() {
    PanelRegistry::Get().Register({
        "settings",
        "Settings",
        "Data",
        DrawSettingsPanel,
        &s_show,
        {"settings", "android settings"}
    });
}
