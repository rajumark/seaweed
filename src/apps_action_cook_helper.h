#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <chrono>

enum class ActionStatus {
    None,
    Running,
    Success,
    Failed
};

struct StatusInfo {
    ActionStatus status = ActionStatus::None;
    std::string message;
    std::string fullDetails;
    std::chrono::steady_clock::time_point timestamp;
    std::string actionName;
    std::string packageName;
    
    bool IsExpired() const {
        auto now = std::chrono::steady_clock::now();
        return (now - timestamp) > std::chrono::seconds(5);
    }
    
    bool IsVisible() const {
        return status != ActionStatus::None && !IsExpired();
    }
};

class AppsActionCookHelper {
public:
    // App management actions (synchronous versions - kept for compatibility)
    static void StartApp(const std::string& packageName);
    static void ForceStopApp(const std::string& packageName);
    static void RestartApp(const std::string& packageName);
    static void UninstallApp(const std::string& packageName);
    static void ClearAppData(const std::string& packageName);
    static void EnableApp(const std::string& packageName);
    static void DisableApp(const std::string& packageName);
    
    // App information actions
    static void OpenAppInfo(const std::string& packageName);
    static void GoHome();
    
    // External actions
    static void ViewAtPlaystore(const std::string& packageName);
    static void ViewAtDesktop(const std::string& packageName);
    static void FindOnline(const std::string& packageName);
    static void CopyPackageName(const std::string& packageName);
    static void DownloadAPK(const std::string& packageName);
    static void ShowMore(const std::string& packageName);
    static void PinApp(const std::string& packageName);
    static void UnpinApp(const std::string& packageName);

    // Async versions of all actions (non-blocking)
    static void StartAppAsync(const std::string& packageName);
    static void ForceStopAppAsync(const std::string& packageName);
    static void RestartAppAsync(const std::string& packageName);
    static void UninstallAppAsync(const std::string& packageName);
    static void ClearAppDataAsync(const std::string& packageName);
    static void EnableAppAsync(const std::string& packageName);
    static void DisableAppAsync(const std::string& packageName);
    static void OpenAppInfoAsync(const std::string& packageName);
    static void GoHomeAsync();
    static void ViewAtPlaystoreAsync(const std::string& packageName);
    static void ViewAtDesktopAsync(const std::string& packageName);
    static void FindOnlineAsync(const std::string& packageName);
    static void DownloadAPKAsync(const std::string& packageName);

    // Background task management
    static bool IsBackgroundTaskRunning();
    static void InitializeBackgroundTaskManager();
    static void ShutdownBackgroundTaskManager();

    // Status management
    static StatusInfo GetCurrentStatus();
    static void CopyStatusToClipboard();

private:
    // Helper functions
    static std::string ExecuteADBCommand(const std::string& command);
    static std::string BuildPlayStoreUrl(const std::string& packageName);
    static std::string BuildFindInMarketUrl(const std::string& packageName);
    static void OpenUrlInAndroidBrowser(const std::string& url);
    static void OpenUrlInDesktopBrowser(const std::string& url);

    // Background task management
    static void BackgroundTaskWorker();
    static void AddBackgroundTask(std::function<void()> task);
    
    // Status management
    static void SetStatus(ActionStatus status, const std::string& actionName, const std::string& packageName, const std::string& message, const std::string& fullDetails = "");
    
    // Static members for background task management
    static std::thread s_backgroundThread;
    static std::atomic<bool> s_backgroundThreadRunning;
    static std::queue<std::function<void()>> s_taskQueue;
    static std::mutex s_taskQueueMutex;
    static std::condition_variable s_taskQueueCV;
    static std::atomic<bool> s_shutdownRequested;
    
    // Static members for status management
    static StatusInfo s_currentStatus;
    static std::mutex s_statusMutex;
};
