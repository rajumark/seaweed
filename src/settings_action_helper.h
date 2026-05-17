#pragma once
#include <string>

// Settings action helper API.
// Provides functions for common settings operations like pinning/unpinning.
namespace SettingsActionHelper {
    // Pins a setting by its ID
    // Returns true on success.
    bool PinSetting(const std::string& settingId);

    // Unpins a setting by its ID
    // Returns true on success.
    bool UnpinSetting(const std::string& settingId);


}
