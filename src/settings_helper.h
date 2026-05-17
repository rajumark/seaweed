#pragma once
#include <string>
#include <vector>

struct SettingsButton {
    std::string id;
    std::string text;
    std::string intent;
};

class SettingsHelper {
public:
    static std::vector<SettingsButton> GetSettingsButtons();
    static bool ExecuteSettingsIntent(const std::string& deviceId, const std::string& settingsId, const std::string& intent);
    static std::vector<SettingsButton> FilterSettingsButtons(const std::string& searchTerm);
    
private:
    static std::vector<SettingsButton> InitializeSettingsButtons();
};
