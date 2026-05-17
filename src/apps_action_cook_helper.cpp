#include "apps_action_cook_helper.h"
#include "apps_pinned_store.h"
#include "global_config.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <process.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

// Static member definitions
std::thread AppsActionCookHelper::s_backgroundThread;
std::atomic<bool> AppsActionCookHelper::s_backgroundThreadRunning(false);
std::queue<std::function<void()>> AppsActionCookHelper::s_taskQueue;
std::mutex AppsActionCookHelper::s_taskQueueMutex;
std::condition_variable AppsActionCookHelper::s_taskQueueCV;
std::atomic<bool> AppsActionCookHelper::s_shutdownRequested(false);

// Static member definitions for status management
StatusInfo AppsActionCookHelper::s_currentStatus;
std::mutex AppsActionCookHelper::s_statusMutex;

std::string AppsActionCookHelper::ExecuteADBCommand(const std::string& command) {
    std::string adbPath = GlobalConfig::GetADBPath();
    if (adbPath.empty()) {
        std::cerr << "ADB path not configured" << std::endl;
        return "";
    }
    
    std::string fullCommand = adbPath + " " + command;
    std::string result;
    
#ifdef _WIN32
    // Windows implementation using _popen
    FILE* pipe = _popen(fullCommand.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    _pclose(pipe);
#else
    // Unix/Linux implementation using popen
    FILE* pipe = popen(fullCommand.c_str(), "r");
    if (!pipe) {
        return "";
    }
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
#endif
    
    return result;
}

void AppsActionCookHelper::StartApp(const std::string& packageName) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " shell monkey -p " + packageName + " -c android.intent.category.LAUNCHER 1";
    std::string result = ExecuteADBCommand(command);
    // Monkey command may return empty on success, so we don't check for empty result
}

void AppsActionCookHelper::ForceStopApp(const std::string& packageName) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " shell am force-stop " + packageName;
    std::string result = ExecuteADBCommand(command);
    // Force stop command typically returns empty on success, so we don't check for empty result
}

void AppsActionCookHelper::RestartApp(const std::string& packageName) {
    ForceStopApp(packageName);
    StartApp(packageName);
}

void AppsActionCookHelper::UninstallApp(const std::string& packageName) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " uninstall " + packageName;
    std::string result = ExecuteADBCommand(command);
    if (result.empty()) {
        throw std::runtime_error("ADB command failed or returned no output");
    }
}

void AppsActionCookHelper::ClearAppData(const std::string& packageName) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " shell pm clear " + packageName;
    std::string result = ExecuteADBCommand(command);
    // Clear data command may return empty on success, so we don't check for empty result
}

void AppsActionCookHelper::EnableApp(const std::string& packageName) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " shell pm enable " + packageName;
    std::string result = ExecuteADBCommand(command);
    // Enable command may return empty on success, so we don't check for empty result
}

void AppsActionCookHelper::DisableApp(const std::string& packageName) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
        std::string command = "-s " + selectedDeviceId + " shell pm disable-user " + packageName;
    std::string result = ExecuteADBCommand(command);
    // Disable command may return empty on success, so we don't check for empty result
}

void AppsActionCookHelper::OpenAppInfo(const std::string& packageName) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " shell am start -a android.settings.APPLICATION_DETAILS_SETTINGS -d package:" + packageName;
    std::string result = ExecuteADBCommand(command);
    // Start activity command may return empty on success, so we don't check for empty result
}

void AppsActionCookHelper::GoHome() {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " shell input keyevent KEYCODE_HOME";
    std::string result = ExecuteADBCommand(command);
    // Input keyevent command may return empty on success, so we don't check for empty result
}

std::string AppsActionCookHelper::BuildPlayStoreUrl(const std::string& packageName) {
    return "https://play.google.com/store/apps/details?id=" + packageName;
}

std::string AppsActionCookHelper::BuildFindInMarketUrl(const std::string& packageName) {
    return "https://www.google.co.in/search?q=download+" + packageName + "+APK";
}

void AppsActionCookHelper::OpenUrlInAndroidBrowser(const std::string& url) {
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        throw std::runtime_error("No device selected");
    }
    std::string command = "-s " + selectedDeviceId + " shell am start -a android.intent.action.VIEW -d " + url;
    std::string result = ExecuteADBCommand(command);
    // Start activity command may return empty on success, so we don't check for empty result
}

void AppsActionCookHelper::OpenUrlInDesktopBrowser(const std::string& url) {
#ifdef _WIN32
    // Windows - let OS handle with default browser
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    // macOS - let OS handle with default browser
    std::string command = "open \"" + url + "\"";
    system(command.c_str());
#else
    // Linux - let OS handle with default browser
    std::string command = "xdg-open \"" + url + "\"";
    system(command.c_str());
#endif
    
}

void AppsActionCookHelper::ViewAtPlaystore(const std::string& packageName) {
    std::string url = BuildPlayStoreUrl(packageName);
    OpenUrlInAndroidBrowser(url);
}

void AppsActionCookHelper::ViewAtDesktop(const std::string& packageName) {
    // Open in desktop browser
    std::string url = BuildPlayStoreUrl(packageName);
    OpenUrlInDesktopBrowser(url);
}

void AppsActionCookHelper::FindOnline(const std::string& packageName) {
    std::string url = BuildFindInMarketUrl(packageName);
    OpenUrlInDesktopBrowser(url);
}

void AppsActionCookHelper::CopyPackageName(const std::string& packageName) {
#ifdef _WIN32
    // Windows clipboard implementation
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        size_t size = (packageName.length() + 1) * sizeof(char);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hGlobal) {
            char* pGlobal = static_cast<char*>(GlobalLock(hGlobal));
            strcpy_s(pGlobal, size, packageName.c_str());
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_TEXT, hGlobal);
            CloseClipboard();
        } else {
            CloseClipboard();
        }
    } else {
    }
#elif defined(__APPLE__)
    // macOS clipboard implementation using pbcopy
    std::string command = "echo -n \"" + packageName + "\" | pbcopy";
    if (system(command.c_str()) == 0) {
    } else {
    }
#else
    // Linux clipboard implementation - try multiple clipboard managers
    const std::vector<std::string> clipboardCommands = {
        "echo -n \"" + packageName + "\" | xclip -selection clipboard",
        "echo -n \"" + packageName + "\" | xsel --clipboard --input",
        "echo -n \"" + packageName + "\" | wl-copy"
    };
    
    bool copied = false;
    for (const auto& cmd : clipboardCommands) {
        if (system((cmd + " 2>/dev/null").c_str()) == 0) {
            copied = true;
            break;
        }
    }
    
    if (copied) {
    } else {
    }
#endif
}

void AppsActionCookHelper::DownloadAPK(const std::string& packageName) {
    // TODO: Implement download APK functionality
}

void AppsActionCookHelper::ShowMore(const std::string& packageName) {
    // TODO: Implement show more functionality
}

void AppsActionCookHelper::PinApp(const std::string& packageName) {
    if (AppsPinnedStore::PinPackage(packageName)) {
    } else {
    }
}

void AppsActionCookHelper::UnpinApp(const std::string& packageName) {
    if (AppsPinnedStore::UnpinPackage(packageName)) {
    } else {
    }
}

// Background task management methods
void AppsActionCookHelper::InitializeBackgroundTaskManager() {
    if (s_backgroundThreadRunning.load()) {
        return; // Already initialized
    }
    
    s_shutdownRequested.store(false);
    s_backgroundThreadRunning.store(true);
    s_backgroundThread = std::thread(&AppsActionCookHelper::BackgroundTaskWorker);
}

void AppsActionCookHelper::ShutdownBackgroundTaskManager() {
    if (!s_backgroundThreadRunning.load()) {
        return; // Not running
    }
    
    s_shutdownRequested.store(true);
    s_taskQueueCV.notify_all();
    
    if (s_backgroundThread.joinable()) {
        s_backgroundThread.join();
    }
    
    s_backgroundThreadRunning.store(false);
}

bool AppsActionCookHelper::IsBackgroundTaskRunning() {
    return s_backgroundThreadRunning.load();
}

void AppsActionCookHelper::BackgroundTaskWorker() {
    while (!s_shutdownRequested.load()) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(s_taskQueueMutex);
            s_taskQueueCV.wait(lock, [] { 
                return !s_taskQueue.empty() || s_shutdownRequested.load(); 
            });
            
            if (s_shutdownRequested.load()) {
                break;
            }
            
            if (!s_taskQueue.empty()) {
                task = std::move(s_taskQueue.front());
                s_taskQueue.pop();
            }
        }
        
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Background task error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error in background task" << std::endl;
            }
        }
    }
}

void AppsActionCookHelper::AddBackgroundTask(std::function<void()> task) {
    if (!s_backgroundThreadRunning.load()) {
        InitializeBackgroundTaskManager();
    }
    
    {
        std::lock_guard<std::mutex> lock(s_taskQueueMutex);
        s_taskQueue.push(std::move(task));
    }
    s_taskQueueCV.notify_one();
}

// Async versions of all action methods
void AppsActionCookHelper::StartAppAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Start App", packageName, "Starting " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            StartApp(packageName);
            SetStatus(ActionStatus::Success, "Start App", packageName, "Successfully started " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to start " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Start App", packageName, "Failed to start " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::ForceStopAppAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Force Stop", packageName, "Force stopping " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            ForceStopApp(packageName);
            SetStatus(ActionStatus::Success, "Force Stop", packageName, "Successfully force stopped " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to force stop " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Force Stop", packageName, "Failed to force stop " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::RestartAppAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Restart", packageName, "Restarting " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            RestartApp(packageName);
            SetStatus(ActionStatus::Success, "Restart", packageName, "Successfully restarted " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to restart " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Restart", packageName, "Failed to restart " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::UninstallAppAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Uninstall", packageName, "Uninstalling " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            UninstallApp(packageName);
            SetStatus(ActionStatus::Success, "Uninstall", packageName, "Successfully uninstalled " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to uninstall " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Uninstall", packageName, "Failed to uninstall " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::ClearAppDataAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Clear Data", packageName, "Clearing data for " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            ClearAppData(packageName);
            SetStatus(ActionStatus::Success, "Clear Data", packageName, "Successfully cleared data for " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to clear data for " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Clear Data", packageName, "Failed to clear data for " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::EnableAppAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Enable", packageName, "Enabling " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            EnableApp(packageName);
            SetStatus(ActionStatus::Success, "Enable", packageName, "Successfully enabled " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to enable " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Enable", packageName, "Failed to enable " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::DisableAppAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Disable", packageName, "Disabling " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            DisableApp(packageName);
            SetStatus(ActionStatus::Success, "Disable", packageName, "Successfully disabled " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to disable " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Disable", packageName, "Failed to disable " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::OpenAppInfoAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Open Info", packageName, "Opening app info for " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            OpenAppInfo(packageName);
            SetStatus(ActionStatus::Success, "Open Info", packageName, "Successfully opened app info for " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to open app info for " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Open Info", packageName, "Failed to open app info for " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::GoHomeAsync() {
    SetStatus(ActionStatus::Running, "Go Home", "", "Going to home screen...");
    AddBackgroundTask([]() {
        try {
            GoHome();
            SetStatus(ActionStatus::Success, "Go Home", "", "Successfully went to home screen");
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to go to home screen: " + std::string(e.what());
            SetStatus(ActionStatus::Failed, "Go Home", "", "Failed to go to home screen", errorMsg);
        }
    });
}

void AppsActionCookHelper::ViewAtPlaystoreAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "View at Playstore", packageName, "Opening Playstore for " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            ViewAtPlaystore(packageName);
            SetStatus(ActionStatus::Success, "View at Playstore", packageName, "Successfully opened Playstore for " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to open Playstore for " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "View at Playstore", packageName, "Failed to open Playstore for " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::ViewAtDesktopAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "View at Desktop", packageName, "Opening desktop view for " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            ViewAtDesktop(packageName);
            SetStatus(ActionStatus::Success, "View at Desktop", packageName, "Successfully opened desktop view for " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to open desktop view for " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "View at Desktop", packageName, "Failed to open desktop view for " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::FindOnlineAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Find Online", packageName, "Searching online for " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            FindOnline(packageName);
            SetStatus(ActionStatus::Success, "Find Online", packageName, "Successfully searched online for " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to search online for " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Find Online", packageName, "Failed to search online for " + packageName, errorMsg);
        }
    });
}

void AppsActionCookHelper::DownloadAPKAsync(const std::string& packageName) {
    SetStatus(ActionStatus::Running, "Download APK", packageName, "Downloading APK for " + packageName + "...");
    AddBackgroundTask([packageName]() {
        try {
            DownloadAPK(packageName);
            SetStatus(ActionStatus::Success, "Download APK", packageName, "Successfully downloaded APK for " + packageName);
        } catch (const std::exception& e) {
            std::string errorMsg = "Failed to download APK for " + packageName + ": " + e.what();
            SetStatus(ActionStatus::Failed, "Download APK", packageName, "Failed to download APK for " + packageName, errorMsg);
        }
    });
}

// Status management methods
void AppsActionCookHelper::SetStatus(ActionStatus status, const std::string& actionName, const std::string& packageName, const std::string& message, const std::string& fullDetails) {
    std::lock_guard<std::mutex> lock(s_statusMutex);
    s_currentStatus.status = status;
    s_currentStatus.actionName = actionName;
    s_currentStatus.packageName = packageName;
    s_currentStatus.message = message;
    s_currentStatus.fullDetails = fullDetails.empty() ? message : fullDetails;
    s_currentStatus.timestamp = std::chrono::steady_clock::now();
}

StatusInfo AppsActionCookHelper::GetCurrentStatus() {
    std::lock_guard<std::mutex> lock(s_statusMutex);
    // Auto-clear expired status
    if (s_currentStatus.IsExpired()) {
        s_currentStatus.status = ActionStatus::None;
        s_currentStatus.message.clear();
        s_currentStatus.fullDetails.clear();
        s_currentStatus.actionName.clear();
        s_currentStatus.packageName.clear();
    }
    return s_currentStatus;
}

void AppsActionCookHelper::CopyStatusToClipboard() {
    std::lock_guard<std::mutex> lock(s_statusMutex);
    if (s_currentStatus.status == ActionStatus::Failed && !s_currentStatus.fullDetails.empty()) {
#ifdef _WIN32
        if (OpenClipboard(nullptr)) {
            EmptyClipboard();
            size_t size = (s_currentStatus.fullDetails.length() + 1) * sizeof(char);
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
            if (hGlobal) {
                char* pGlobal = static_cast<char*>(GlobalLock(hGlobal));
                strcpy_s(pGlobal, size, s_currentStatus.fullDetails.c_str());
                GlobalUnlock(hGlobal);
                SetClipboardData(CF_TEXT, hGlobal);
            }
            CloseClipboard();
        }
#else
        // For non-Windows platforms, you might need to implement clipboard functionality
        // This is a placeholder - you may want to use xclip, xsel, or similar tools
#endif
    }
}
