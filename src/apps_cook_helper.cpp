#include "apps_cook_helper.h"
#include "device_manager.h"
#include "global_config.h"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <thread>

namespace AppsCookHelper {

// Thread-safe data structures
static std::vector<std::string> g_packages;
static std::string g_lastError;
static int g_selectedFilter = 0; // 0=All, 1=System, 2=User

// Threading support
static std::mutex g_dataMutex;
static std::atomic<bool> g_isLoading(false);
static std::thread g_backgroundThread;
static std::atomic<bool> g_shouldStop(false);

// Internal function for actual loading (runs in background thread)
static void LoadPackagesListInternal() {
    std::vector<std::string> tempPackages;
    std::string tempError;

    // Get selected device ID
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        tempError = "No device selected";
    } else {
        // Get ADB path
        std::string adbPath = GlobalConfig::GetADBPath();
        if (adbPath.empty()) {
            tempError = "ADB path not configured";
        } else {
            // Build command based on selected filter
            std::string command;
            switch (g_selectedFilter) {
                case 1: // System Apps
                    command = "\"" + adbPath + "\" -s " + selectedDeviceId + " shell pm list packages -s";
                    break;
                case 2: // User Apps
                    command = "\"" + adbPath + "\" -s " + selectedDeviceId + " shell pm list packages -3";
                    break;
                default: // All Apps
                    command = "\"" + adbPath + "\" -s " + selectedDeviceId + " shell pm list packages";
                    break;
            }

            // Execute via DeviceManager
            std::string output = DeviceManager::GetInstance().ExecuteCommand(command);

            if (output.empty()) {
                tempError = "No output from ADB command";
            } else {
                // Parse packages
                std::istringstream iss(output);
                std::string line;
                while (std::getline(iss, line)) {
                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                    if (line.rfind("package:", 0) == 0) {
                        tempPackages.push_back(line.substr(8)); // strip "package:"
                    }
                }

                // Sort alphabetically
                std::sort(tempPackages.begin(), tempPackages.end());
            }
        }
    }

    // Update data with thread safety
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        g_packages = std::move(tempPackages);
        g_lastError = std::move(tempError);
    }
}

void LoadPackagesListAsync() {
    // Don't start if already loading
    if (g_isLoading.load()) {
        return;
    }

    // Clean up any existing thread
    if (g_backgroundThread.joinable()) {
        g_backgroundThread.join();
    }

    g_isLoading.store(true);
    g_shouldStop.store(false);

    // Start background thread
    g_backgroundThread = std::thread([]() {
        LoadPackagesListInternal();
        g_isLoading.store(false);
    });
}

bool IsLoading() {
    return g_isLoading.load();
}

void LoadPackagesList() {
    // Synchronous version - for immediate use if needed
    LoadPackagesListInternal();
}

void SetFilter(int filter) {
    std::lock_guard<std::mutex> lock(g_dataMutex);
    g_selectedFilter = filter;
}

int GetFilter() {
    std::lock_guard<std::mutex> lock(g_dataMutex);
    return g_selectedFilter;
}

const std::vector<std::string>& GetPackages() {
    std::lock_guard<std::mutex> lock(g_dataMutex);
    return g_packages;
}

const std::string& GetLastError() {
    std::lock_guard<std::mutex> lock(g_dataMutex);
    return g_lastError;
}

std::vector<std::string> GetFilteredPackages(const std::string& searchTerm) {
    std::vector<std::string> filteredPackages;
    
    // Get packages with thread safety
    std::vector<std::string> packages;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        packages = g_packages; // Copy for filtering
    }
    
    if (searchTerm.empty()) {
        // If no search term, show all packages
        filteredPackages = packages;
    } else {
        // Filter packages that contain the search term (case-insensitive)
        std::string lowerSearchTerm = searchTerm;
        std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), lowerSearchTerm.begin(), ::tolower);
        
        for (const auto& pkg : packages) {
            std::string lowerPkg = pkg;
            std::transform(lowerPkg.begin(), lowerPkg.end(), lowerPkg.begin(), ::tolower);
            if (lowerPkg.find(lowerSearchTerm) != std::string::npos) {
                filteredPackages.push_back(pkg);
            }
        }
    }
    
    return filteredPackages;
}

void Cleanup() {
    g_shouldStop.store(true);
    if (g_backgroundThread.joinable()) {
        g_backgroundThread.join();
    }
}

} // namespace AppsCookHelper
