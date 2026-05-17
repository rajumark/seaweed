#include "settings_pinned_store.h"
#include "setup.h" // for GetADBKingPath()

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <mutex>

namespace {
    const char* kPinnedFileName = "pinned_settings.txt";
    
    // Cache for pinned settings
    std::unordered_set<std::string> g_cachedPinnedSettings;
    std::mutex g_cacheMutex;
    bool g_cacheValid = false;

    std::string GetPinnedFilePath() {
        std::string base = GetADBKingPath();
#ifdef _WIN32
        return base + "\\" + kPinnedFileName;
#else
        return base + "/" + kPinnedFileName;
#endif
    }

    // Check if file has been modified since last read
    bool IsFileModified() {
        try {
            const std::string path = GetPinnedFilePath();
            if (!std::filesystem::exists(path)) {
                return g_cacheValid; // File doesn't exist, but cache might be valid (empty)
            }
            
            // For C++17 compatibility, we'll use a simpler approach
            // Just check if the file exists and assume it's modified if cache is invalid
            return !g_cacheValid;
        } catch (...) {
            return true; // Assume modified on error
        }
    }

    // Load all pinned setting IDs from file
    std::unordered_set<std::string> LoadPinnedSettings() {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        
        // Check if cache is still valid
        if (g_cacheValid && !IsFileModified()) {
            return g_cachedPinnedSettings;
        }
        
        // Load from file
        std::unordered_set<std::string> pinnedSettings;
        try {
            const std::string path = GetPinnedFilePath();
            std::ifstream in(path);
            if (!in.is_open()) {
                // Ensure directory exists; file will be created on first write
                std::filesystem::create_directories(GetADBKingPath());
                g_cachedPinnedSettings.clear();
                g_cacheValid = true;
                return pinnedSettings;
            }

            std::string line;
            while (std::getline(in, line)) {
                // Skip empty or comment lines
                if (line.empty() || line[0] == '#') continue;

                // Trim whitespace
                auto trim = [](std::string& s) {
                    size_t start = s.find_first_not_of(" \t\r\n");
                    size_t end = s.find_last_not_of(" \t\r\n");
                    if (start == std::string::npos || end == std::string::npos) { s.clear(); return; }
                    s = s.substr(start, end - start + 1);
                };
                trim(line);
                
                if (!line.empty()) {
                    pinnedSettings.insert(line);
                }
            }
            
            // Update cache
            g_cachedPinnedSettings = pinnedSettings;
            g_cacheValid = true;
        } catch (...) {
            // Swallow and return what we have
        }
        return pinnedSettings;
    }

    // Save all pinned setting IDs to file
    bool SavePinnedSettings(const std::unordered_set<std::string>& pinnedSettings) {
        try {
            std::filesystem::create_directories(GetADBKingPath());
            const std::string path = GetPinnedFilePath();
            std::ofstream out(path, std::ios::trunc);
            if (!out.is_open()) return false;
            
            out << "# ADBKing pinned settings list" << '\n';
            for (const auto& settingId : pinnedSettings) {
                out << settingId << '\n';
            }
            
            // Update cache after successful save
            {
                std::lock_guard<std::mutex> lock(g_cacheMutex);
                g_cachedPinnedSettings = pinnedSettings;
                g_cacheValid = true;
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
}

namespace SettingsPinnedStore {

bool PinSetting(const std::string& settingId) {
    if (settingId.empty()) return false;
    
    auto pinnedSettings = LoadPinnedSettings();
    pinnedSettings.insert(settingId);
    return SavePinnedSettings(pinnedSettings);
}

bool UnpinSetting(const std::string& settingId) {
    if (settingId.empty()) return false;
    
    auto pinnedSettings = LoadPinnedSettings();
    auto it = pinnedSettings.find(settingId);
    if (it == pinnedSettings.end()) return false;
    
    pinnedSettings.erase(it);
    return SavePinnedSettings(pinnedSettings);
}

bool IsSettingPinned(const std::string& settingId) {
    if (settingId.empty()) return false;
    
    auto pinnedSettings = LoadPinnedSettings();
    return pinnedSettings.find(settingId) != pinnedSettings.end();
}

std::vector<std::string> GetPinnedSettings() {
    auto pinnedSettings = LoadPinnedSettings();
    std::vector<std::string> result;
    result.reserve(pinnedSettings.size());
    
    for (const auto& settingId : pinnedSettings) {
        result.push_back(settingId);
    }
    
    // Sort for consistent ordering
    std::sort(result.begin(), result.end());
    return result;
}

size_t GetPinnedCount() {
    auto pinnedSettings = LoadPinnedSettings();
    return pinnedSettings.size();
}

void ClearCache() {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    g_cacheValid = false;
    g_cachedPinnedSettings.clear();
}

}
