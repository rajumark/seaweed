#pragma once

#include <string>
#include <vector>

// Pinned settings store API.
// Manages a list of pinned setting IDs stored in a separate text file.
// This module handles pinning/unpinning settings and retrieving the pinned list.
namespace SettingsPinnedStore {
    // Pins a setting ID (adds it to the pinned list)
    // Returns true on success.
    bool PinSetting(const std::string& settingId);

    // Unpins a setting ID (removes it from the pinned list)
    // Returns true if the setting was pinned and is now removed.
    bool UnpinSetting(const std::string& settingId);

    // Checks if a setting is pinned
    // Returns true if the setting is in the pinned list.
    bool IsSettingPinned(const std::string& settingId);

    // Gets all pinned setting IDs
    // Returns a vector of all currently pinned setting IDs.
    std::vector<std::string> GetPinnedSettings();

    // Gets the count of pinned settings
    // Returns the number of currently pinned settings.
    size_t GetPinnedCount();
    
    // Clear the internal cache (call this when you want to force a reload)
    void ClearCache();
}
