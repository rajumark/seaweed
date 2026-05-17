#pragma once

#include <string>

// Simple persistent key-value store API.
// Values are stored in a text file located under the app's data folder
// returned by GetADBKingPath(), in a file named "key_value_store.txt".
// This module only contains generic storage logic (set/get/delete) and
// does not contain any app-specific behavior.
namespace KeyValueStore {
    // Sets a string value for the given key. Creates/updates the storage file.
    // Returns true on success.
    bool SetString(const std::string& key, const std::string& value);

    // Gets a string value for the given key. Returns defaultValue if missing
    // or on any error.
    std::string GetString(const std::string& key, const std::string& defaultValue);

    // Deletes a key from the store. Returns true if the key existed and was removed.
    bool DeleteKey(const std::string& key);

    // Convenience typed helpers
    bool SetFloat(const std::string& key, float value);
    float GetFloat(const std::string& key, float defaultValue);

    bool SetInt(const std::string& key, int value);
    int GetInt(const std::string& key, int defaultValue);

    bool SetBool(const std::string& key, bool value);
    bool GetBool(const std::string& key, bool defaultValue);
}


