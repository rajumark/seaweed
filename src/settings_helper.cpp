#include "settings_helper.h"
#include "global_config.h"
#include <iostream>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <cstdlib>
#else
#include <cstdlib>
#endif

std::vector<SettingsButton> SettingsHelper::GetSettingsButtons() {
    static std::vector<SettingsButton> buttons = InitializeSettingsButtons();
    return buttons;
}

std::vector<SettingsButton> SettingsHelper::FilterSettingsButtons(const std::string& searchTerm) {
    if (searchTerm.empty()) {
        return GetSettingsButtons();
    }
    
    std::vector<SettingsButton> filtered;
    std::string lowerSearchTerm = searchTerm;
    std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), ::tolower);
    
    for (const auto& button : GetSettingsButtons()) {
        std::string lowerText = button.text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
        
        if (lowerText.find(lowerSearchTerm) != std::string::npos) {
            filtered.push_back(button);
        }
    }
    
    return filtered;
}

bool SettingsHelper::ExecuteSettingsIntent(const std::string& deviceId, const std::string& settingsId, const std::string& intent) {
    if (deviceId.empty()) {
        std::cerr << "No device selected" << std::endl;
        return false;
    }
    
    std::string adbPath = GlobalConfig::GetADBPath();
    if (adbPath.empty()) {
        std::cerr << "ADB path not configured" << std::endl;
        return false;
    }
    
    // Build the ADB command
    std::string command = "\"" + adbPath + "\" -s " + deviceId + " shell am start -a " + intent;
    
    // Execute the command
    int result = system(command.c_str());
    
    if (result == 0) {
        return true;
    } else {
        std::cerr << "Failed to execute settings intent: " << settingsId << std::endl;
        return false;
    }
}

std::vector<SettingsButton> SettingsHelper::InitializeSettingsButtons() {
    return {
        {"settings", "Settings", "android.settings.SETTINGS"},
        {"apn_settings", "APN Settings", "android.settings.APN_SETTINGS"},
        {"location_source_settings", "Location Source Settings", "android.settings.LOCATION_SOURCE_SETTINGS"},
        {"location_controller_extra_package_settings", "Location Controller Extra Package Settings", "android.settings.LOCATION_CONTROLLER_EXTRA_PACKAGE_SETTINGS"},
        {"location_scanning_settings", "Location Scanning Settings", "android.settings.LOCATION_SCANNING_SETTINGS"},
        {"manage_cloned_apps_settings", "Manage Cloned Apps Settings", "android.settings.MANAGE_CLONED_APPS_SETTINGS"},
        {"user_settings", "User Settings", "android.settings.USER_SETTINGS"},
        {"wireless_settings", "Wireless Settings", "android.settings.WIRELESS_SETTINGS"},
        {"tether_provisioning_ui", "Tether Provisioning UI", "android.settings.TETHER_PROVISIONING_UI"},
        {"tether_unsupported_carrier_ui", "Tether Unsupported Carrier UI", "android.settings.TETHER_UNSUPPORTED_CARRIER_UI"},
        {"airplane_mode_settings", "Airplane Mode Settings", "android.settings.AIRPLANE_MODE_SETTINGS"},
        {"mobile_data_usage", "Mobile Data Usage", "android.settings.MOBILE_DATA_USAGE"},
        {"one_handed_settings", "One-Handed Settings", "android.settings.action.ONE_HANDED_SETTINGS"},
        {"voice_control_airplane_mode", "Voice Control Airplane Mode", "android.settings.VOICE_CONTROL_AIRPLANE_MODE"},
        {"accessibility_settings", "Accessibility Settings", "android.settings.ACCESSIBILITY_SETTINGS"},
        {"accessibility_details_settings", "Accessibility Details Settings", "android.settings.ACCESSIBILITY_DETAILS_SETTINGS"},
        {"accessibility_color_motion_settings", "Accessibility Color Motion Settings", "android.settings.ACCESSIBILITY_COLOR_MOTION_SETTINGS"},
        {"reduce_bright_colors_settings", "Reduce Bright Colors Settings", "android.settings.REDUCE_BRIGHT_COLORS_SETTINGS"},
        {"accessibility_color_space_settings", "Accessibility Color Space Settings", "com.android.settings.ACCESSIBILITY_COLOR_SPACE_SETTINGS"},
        {"color_inversion_settings", "Color Inversion Settings", "android.settings.COLOR_INVERSION_SETTINGS"},
        {"text_reading_settings", "Text Reading Settings", "android.settings.TEXT_READING_SETTINGS"},
        {"usage_access_settings", "Usage Access Settings", "android.settings.USAGE_ACCESS_SETTINGS"},
        {"usage_access_config", "Usage Access Config", "android.intent.category.USAGE_ACCESS_CONFIG"},
        {"usage_access_reason", "Usage Access Reason", "android.settings.metadata.USAGE_ACCESS_REASON"},
        {"security_settings", "Security Settings", "android.settings.SECURITY_SETTINGS"},
        {"manage_unknown_app_sources", "Manage Unknown App Sources", "android.settings.MANAGE_UNKNOWN_APP_SOURCES"},
        {"request_schedule_exact_alarm", "Request Schedule Exact Alarm", "android.settings.REQUEST_SCHEDULE_EXACT_ALARM"},
        {"request_manage_media", "Request Manage Media", "android.settings.REQUEST_MANAGE_MEDIA"},
        {"manage_app_long_running_jobs", "Manage App Long Running Jobs", "android.settings.MANAGE_APP_LONG_RUNNING_JOBS"},
        {"manage_cross_profile_access", "Manage Cross Profile Access", "android.settings.MANAGE_CROSS_PROFILE_ACCESS"},
        {"app_open_by_default_settings", "App Open By Default Settings", "android.settings.APP_OPEN_BY_DEFAULT_SETTINGS"},
        {"trusted_credentials_user", "Trusted Credentials User", "com.android.settings.TRUSTED_CREDENTIALS_USER"},
        {"monitoring_cert_info", "Monitoring Cert Info", "com.android.settings.MONITORING_CERT_INFO"},
        {"privacy_settings", "Privacy Settings", "android.settings.PRIVACY_SETTINGS"},
        {"vpn_settings", "VPN Settings", "android.settings.VPN_SETTINGS"},
        {"wifi_settings", "WiFi Settings", "android.settings.WIFI_SETTINGS"},
        {"advanced_memory_protection_settings", "Advanced Memory Protection Settings", "android.settings.ADVANCED_MEMORY_PROTECTION_SETTINGS"},
        {"wifi_ip_settings", "WiFi IP Settings", "android.settings.WIFI_IP_SETTINGS"},
        {"process_wifi_easy_connect_uri", "Process WiFi Easy Connect URI", "android.settings.PROCESS_WIFI_EASY_CONNECT_URI"},
        {"data_usage_settings", "Data Usage Settings", "android.settings.DATA_USAGE_SETTINGS"},
        {"bluetooth_settings", "Bluetooth Settings", "android.settings.BLUETOOTH_SETTINGS"},
        {"app_search_settings", "App Search Settings", "android.settings.APP_SEARCH_SETTINGS"},
        {"assist_gesture_settings", "Assist Gesture Settings", "android.settings.ASSIST_GESTURE_SETTINGS"},
        {"fingerprint_enroll", "Fingerprint Enroll", "android.settings.FINGERPRINT_ENROLL"},
        {"biometric_enroll", "Biometric Enroll", "android.settings.BIOMETRIC_ENROLL"},
        {"cast_settings", "Cast Settings", "android.settings.CAST_SETTINGS"},
        {"date_settings", "Date Settings", "android.settings.DATE_SETTINGS"},
        {"sound_settings", "Sound Settings", "android.settings.SOUND_SETTINGS"},
        {"display_settings", "Display Settings", "android.settings.DISPLAY_SETTINGS"},
        {"auto_rotate_settings", "Auto Rotate Settings", "android.settings.AUTO_ROTATE_SETTINGS"},
        {"night_display_settings", "Night Display Settings", "android.settings.NIGHT_DISPLAY_SETTINGS"},
        {"dark_theme_settings", "Dark Theme Settings", "android.settings.DARK_THEME_SETTINGS"},
        {"locale_settings", "Locale Settings", "android.settings.LOCALE_SETTINGS"},
        {"app_locale_settings", "App Locale Settings", "android.settings.APP_LOCALE_SETTINGS"},
        {"regional_preferences_settings", "Regional Preferences Settings", "android.settings.REGIONAL_PREFERENCES_SETTINGS"},
        {"lock_screen_settings", "Lock Screen Settings", "android.settings.LOCK_SCREEN_SETTINGS"},
        {"bluetooth_pairing_settings", "Bluetooth Pairing Settings", "android.settings.BLUETOOTH_PAIRING_SETTINGS"},
        {"voice_input_settings", "Voice Input Settings", "android.settings.VOICE_INPUT_SETTINGS"},
        {"input_method_settings", "Input Method Settings", "android.settings.INPUT_METHOD_SETTINGS"},
        {"input_method_subtype_settings", "Input Method Subtype Settings", "android.settings.INPUT_METHOD_SUBTYPE_SETTINGS"},
        {"user_dictionary_settings", "User Dictionary Settings", "android.settings.USER_DICTIONARY_SETTINGS"},
        {"hard_keyboard_settings", "Hard Keyboard Settings", "android.settings.HARD_KEYBOARD_SETTINGS"},
        {"user_dictionary_insert", "User Dictionary Insert", "com.android.settings.USER_DICTIONARY_INSERT"},
        {"application_settings", "Application Settings", "android.settings.APPLICATION_SETTINGS"},
        {"application_development_settings", "Application Development Settings", "android.settings.APPLICATION_DEVELOPMENT_SETTINGS"},
        {"quick_launch_settings", "Quick Launch Settings", "android.settings.QUICK_LAUNCH_SETTINGS"},
        {"manage_applications_settings", "Manage Applications Settings", "android.settings.MANAGE_APPLICATIONS_SETTINGS"},
        {"manage_all_applications_settings", "Manage All Applications Settings", "android.settings.MANAGE_ALL_APPLICATIONS_SETTINGS"},
        {"manage_all_sim_profiles_settings", "Manage All SIM Profiles Settings", "android.settings.MANAGE_ALL_SIM_PROFILES_SETTINGS"},
        {"manage_overlay_permission", "Manage Overlay Permission", "android.settings.action.MANAGE_OVERLAY_PERMISSION"},
        {"manage_app_overlay_permission", "Manage App Overlay Permission", "android.settings.MANAGE_APP_OVERLAY_PERMISSION"},
        {"manage_write_settings", "Manage Write Settings", "android.settings.action.MANAGE_WRITE_SETTINGS"},
        {"app_usage_settings", "App Usage Settings", "android.settings.action.APP_USAGE_SETTINGS"},
        {"application_details_settings", "Application Details Settings", "android.settings.APPLICATION_DETAILS_SETTINGS"},
        {"foreground_services_settings", "Foreground Services Settings", "android.settings.FOREGROUND_SERVICES_SETTINGS"},
        {"ignore_battery_optimization_settings", "Ignore Battery Optimization Settings", "android.settings.IGNORE_BATTERY_OPTIMIZATION_SETTINGS"},
        {"request_ignore_battery_optimizations", "Request Ignore Battery Optimizations", "android.settings.REQUEST_IGNORE_BATTERY_OPTIMIZATIONS"},
        {"view_advanced_power_usage_detail", "View Advanced Power Usage Detail", "android.settings.VIEW_ADVANCED_POWER_USAGE_DETAIL"},
        {"ignore_background_data_restrictions_settings", "Ignore Background Data Restrictions Settings", "android.settings.IGNORE_BACKGROUND_DATA_RESTRICTIONS_SETTINGS"},
        {"app_ops_settings", "App Ops Settings", "android.settings.APP_OPS_SETTINGS"},
        {"system_update_settings", "System Update Settings", "android.settings.SYSTEM_UPDATE_SETTINGS"},
        {"managed_profile_settings", "Managed Profile Settings", "android.settings.MANAGED_PROFILE_SETTINGS"},
        {"sync_settings", "Sync Settings", "android.settings.SYNC_SETTINGS"},
        {"add_account_settings", "Add Account Settings", "android.settings.ADD_ACCOUNT_SETTINGS"},
        {"data_saver_settings", "Data Saver Settings", "android.settings.DATA_SAVER_SETTINGS"},
        {"network_operator_settings", "Network Operator Settings", "android.settings.NETWORK_OPERATOR_SETTINGS"},
        {"data_roaming_settings", "Data Roaming Settings", "android.settings.DATA_ROAMING_SETTINGS"},
        {"internal_storage_settings", "Internal Storage Settings", "android.settings.INTERNAL_STORAGE_SETTINGS"},
        {"memory_card_settings", "Memory Card Settings", "android.settings.MEMORY_CARD_SETTINGS"},
        {"search_settings", "Search Settings", "android.search.action.SEARCH_SETTINGS"},
        {"device_info_settings", "Device Info Settings", "android.settings.DEVICE_INFO_SETTINGS"},
        {"nfc_settings", "NFC Settings", "android.settings.NFC_SETTINGS"},
        {"nfc_sharing_settings", "NFC Sharing Settings", "android.settings.NFCSHARING_SETTINGS"},
        {"nfc_payment_settings", "NFC Payment Settings", "android.settings.NFC_PAYMENT_SETTINGS"},
        {"dream_settings", "Dream Settings", "android.settings.DREAM_SETTINGS"},
        {"communal_settings", "Communal Settings", "android.settings.COMMUNAL_SETTINGS"},
        {"notification_assistant_settings", "Notification Assistant Settings", "android.settings.NOTIFICATION_ASSISTANT_SETTINGS"},
        {"notification_listener_settings", "Notification Listener Settings", "android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS"},
        {"notification_listener_detail_settings", "Notification Listener Detail Settings", "android.settings.NOTIFICATION_LISTENER_DETAIL_SETTINGS"},
        {"notification_policy_access_settings", "Notification Policy Access Settings", "android.settings.NOTIFICATION_POLICY_ACCESS_SETTINGS"},
        {"notification_policy_access_detail_settings", "Notification Policy Access Detail Settings", "android.settings.NOTIFICATION_POLICY_ACCESS_DETAIL_SETTINGS"},
        {"condition_provider_settings", "Condition Provider Settings", "android.settings.ACTION_CONDITION_PROVIDER_SETTINGS"},
        {"captioning_settings", "Captioning Settings", "android.settings.CAPTIONING_SETTINGS"},
        {"print_settings", "Print Settings", "android.settings.ACTION_PRINT_SETTINGS"},
        {"zen_mode_settings", "Zen Mode Settings", "android.settings.ZEN_MODE_SETTINGS"},
        {"zen_mode_blocked_effects_settings", "Zen Mode Blocked Effects Settings", "android.settings.ZEN_MODE_BLOCKED_EFFECTS_SETTINGS"},
        {"zen_mode_onboarding", "Zen Mode Onboarding", "android.settings.ZEN_MODE_ONBOARDING"},
        {"zen_mode_priority_settings", "Zen Mode Priority Settings", "android.settings.ZEN_MODE_PRIORITY_SETTINGS"},
        {"zen_mode_automation_settings", "Zen Mode Automation Settings", "android.settings.ZEN_MODE_AUTOMATION_SETTINGS"},
        {"voice_control_do_not_disturb_mode", "Voice Control Do Not Disturb Mode", "android.settings.VOICE_CONTROL_DO_NOT_DISTURB_MODE"},
        {"zen_mode_schedule_rule_settings", "Zen Mode Schedule Rule Settings", "android.settings.ZEN_MODE_SCHEDULE_RULE_SETTINGS"},
        {"zen_mode_event_rule_settings", "Zen Mode Event Rule Settings", "android.settings.ZEN_MODE_EVENT_RULE_SETTINGS"},
        {"zen_mode_external_rule_settings", "Zen Mode External Rule Settings", "android.settings.ZEN_MODE_EXTERNAL_RULE_SETTINGS"},
        {"show_regulatory_info", "Show Regulatory Info", "android.settings.SHOW_REGULATORY_INFO"},
        {"device_name", "Device Name", "android.settings.DEVICE_NAME"},
        {"pairing_settings", "Pairing Settings", "android.settings.PAIRING_SETTINGS"},
        {"battery_saver_settings", "Battery Saver Settings", "android.settings.BATTERY_SAVER_SETTINGS"},
        {"voice_control_battery_saver_mode", "Voice Control Battery Saver Mode", "android.settings.VOICE_CONTROL_BATTERY_SAVER_MODE"},
        {"home_settings", "Home Settings", "android.settings.HOME_SETTINGS"},
        {"manage_default_apps_settings", "Manage Default Apps Settings", "android.settings.MANAGE_DEFAULT_APPS_SETTINGS"},
        {"manage_more_default_apps_settings", "Manage More Default Apps Settings", "android.settings.MANAGE_MORE_DEFAULT_APPS_SETTINGS"},
        {"notification_settings", "Notification Settings", "android.settings.NOTIFICATION_SETTINGS"},
        {"conversation_settings", "Conversation Settings", "android.settings.CONVERSATION_SETTINGS"},
        {"notification_history", "Notification History", "android.settings.NOTIFICATION_HISTORY"},
        {"all_apps_notification_settings", "All Apps Notification Settings", "android.settings.ALL_APPS_NOTIFICATION_SETTINGS"},
        {"all_apps_notification_settings_for_review", "All Apps Notification Settings For Review", "android.settings.ALL_APPS_NOTIFICATION_SETTINGS_FOR_REVIEW"},
        {"app_notification_settings", "App Notification Settings", "android.settings.APP_NOTIFICATION_SETTINGS"},
        {"channel_notification_settings", "Channel Notification Settings", "android.settings.CHANNEL_NOTIFICATION_SETTINGS"},
        {"app_notification_bubble_settings", "App Notification Bubble Settings", "android.settings.APP_NOTIFICATION_BUBBLE_SETTINGS"},
        {"action_app_notification_redaction", "Action App Notification Redaction", "android.settings.ACTION_APP_NOTIFICATION_REDACTION"},
        {"action_power_menu_settings", "Action Power Menu Settings", "android.settings.ACTION_POWER_MENU_SETTINGS"},
        {"action_device_controls_settings", "Action Device Controls Settings", "android.settings.ACTION_DEVICE_CONTROLS_SETTINGS"},
        {"action_media_controls_settings", "Action Media Controls Settings", "android.settings.ACTION_MEDIA_CONTROLS_SETTINGS"},
        {"show_admin_support_details", "Show Admin Support Details", "android.settings.SHOW_ADMIN_SUPPORT_DETAILS"},
        {"manage_supervisor_restricted_setting", "Manage Supervisor Restricted Setting", "android.settings.MANAGE_SUPERVISOR_RESTRICTED_SETTING"},
        {"show_remote_bugreport_dialog", "Show Remote Bugreport Dialog", "android.settings.SHOW_REMOTE_BUGREPORT_DIALOG"},
        {"vr_listener_settings", "VR Listener Settings", "android.settings.VR_LISTENER_SETTINGS"},
        {"picture_in_picture_settings", "Picture In Picture Settings", "android.settings.PICTURE_IN_PICTURE_SETTINGS"},
        {"storage_manager_settings", "Storage Manager Settings", "android.settings.STORAGE_MANAGER_SETTINGS"},
        {"webview_settings", "Webview Settings", "android.settings.WEBVIEW_SETTINGS"},
        {"enterprise_privacy_settings", "Enterprise Privacy Settings", "android.settings.ENTERPRISE_PRIVACY_SETTINGS"},
        {"show_work_policy_info", "Show Work Policy Info", "android.settings.SHOW_WORK_POLICY_INFO"},
        {"request_set_autofill_service", "Request Set Autofill Service", "android.settings.REQUEST_SET_AUTOFILL_SERVICE"},
        {"quick_access_wallet_settings", "Quick Access Wallet Settings", "android.settings.QUICK_ACCESS_WALLET_SETTINGS"},
        {"storage_volume_access_settings", "Storage Volume Access Settings", "android.settings.STORAGE_VOLUME_ACCESS_SETTINGS"},
        {"request_enable_content_capture", "Request Enable Content Capture", "android.settings.REQUEST_ENABLE_CONTENT_CAPTURE"},
        {"manage_domain_urls", "Manage Domain URLs", "android.settings.MANAGE_DOMAIN_URLS"},
        {"tether_settings", "Tether Settings", "android.settings.TETHER_SETTINGS"},
        {"wifi_tether_settings", "WiFi Tether Settings", "com.android.settings.WIFI_TETHER_SETTINGS"},
        {"enable_mms_data_request", "Enable MMS Data Request", "android.settings.ENABLE_MMS_DATA_REQUEST"},
        {"show_restricted_setting_dialog", "Show Restricted Setting Dialog", "android.settings.SHOW_RESTRICTED_SETTING_DIALOG"},
        {"extra_enable_mms_data_request_reason", "Extra Enable MMS Data Request Reason", "android.settings.extra.ENABLE_MMS_DATA_REQUEST_REASON"},
        {"mms_message_setting", "MMS Message Setting", "android.settings.MMS_MESSAGE_SETTING"},
        {"bedtime_settings", "Bedtime Settings", "android.settings.BEDTIME_SETTINGS"}
    };
}
