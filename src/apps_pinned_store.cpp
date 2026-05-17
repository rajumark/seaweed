#include "apps_pinned_store.h"
#include "setup.h" // for GetADBKingPath()

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <mutex>

namespace {
    const char* kPinnedFileName = "pinned_apps.txt";
    
    // Cache for pinned packages
    std::unordered_set<std::string> g_cachedPinnedPackages;
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

    // Load all pinned package names from file
    std::unordered_set<std::string> LoadPinnedPackages() {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        
        // Check if cache is still valid
        if (g_cacheValid && !IsFileModified()) {
            return g_cachedPinnedPackages;
        }
        
        // Load from file
        std::unordered_set<std::string> pinnedPackages;
        try {
            const std::string path = GetPinnedFilePath();
            std::ifstream in(path);
            if (!in.is_open()) {
                // Ensure directory exists; file will be created on first write
                std::filesystem::create_directories(GetADBKingPath());
                g_cachedPinnedPackages.clear();
                g_cacheValid = true;
                return pinnedPackages;
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
                    pinnedPackages.insert(line);
                }
            }
            
            // Update cache
            g_cachedPinnedPackages = pinnedPackages;
            g_cacheValid = true;
        } catch (...) {
            // Swallow and return what we have
        }
        return pinnedPackages;
    }

    // Save all pinned package names to file
    bool SavePinnedPackages(const std::unordered_set<std::string>& pinnedPackages) {
        try {
            std::filesystem::create_directories(GetADBKingPath());
            const std::string path = GetPinnedFilePath();
            std::ofstream out(path, std::ios::trunc);
            if (!out.is_open()) return false;
            
            out << "# ADBKing pinned apps list" << '\n';
            for (const auto& packageName : pinnedPackages) {
                out << packageName << '\n';
            }
            
            // Update cache after successful save
            {
                std::lock_guard<std::mutex> lock(g_cacheMutex);
                g_cachedPinnedPackages = pinnedPackages;
                g_cacheValid = true;
            }
            
            return true;
        } catch (...) {
            return false;
        }
    }
}

namespace AppsPinnedStore {

bool PinPackage(const std::string& packageName) {
    if (packageName.empty()) return false;
    
    auto pinnedPackages = LoadPinnedPackages();
    pinnedPackages.insert(packageName);
    return SavePinnedPackages(pinnedPackages);
}

bool UnpinPackage(const std::string& packageName) {
    if (packageName.empty()) return false;
    
    auto pinnedPackages = LoadPinnedPackages();
    auto it = pinnedPackages.find(packageName);
    if (it == pinnedPackages.end()) return false;
    
    pinnedPackages.erase(it);
    return SavePinnedPackages(pinnedPackages);
}

bool IsPackagePinned(const std::string& packageName) {
    if (packageName.empty()) return false;
    
    auto pinnedPackages = LoadPinnedPackages();
    return pinnedPackages.find(packageName) != pinnedPackages.end();
}

std::vector<std::string> GetPinnedPackages() {
    auto pinnedPackages = LoadPinnedPackages();
    std::vector<std::string> result;
    result.reserve(pinnedPackages.size());
    
    for (const auto& packageName : pinnedPackages) {
        result.push_back(packageName);
    }
    
    // Sort for consistent ordering
    std::sort(result.begin(), result.end());
    return result;
}

size_t GetPinnedCount() {
    auto pinnedPackages = LoadPinnedPackages();
    return pinnedPackages.size();
}

void ClearCache() {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    g_cacheValid = false;
    g_cachedPinnedPackages.clear();
}

}
