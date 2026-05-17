#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

// Device information structure
struct DeviceInfo {
    std::string deviceId;
    std::string deviceName;
    std::string status;
    bool isConnected;
    
    DeviceInfo() : isConnected(false) {}
    DeviceInfo(const std::string& id, const std::string& name, const std::string& stat) 
        : deviceId(id), deviceName(name), status(stat), isConnected(true) {}
};

class DeviceManager {
public:
    static DeviceManager& GetInstance();
    
    // Initialize the device manager
    void Initialize(const std::string& adbPath);
    
    // Start background device monitoring
    void StartMonitoring();
    
    // Stop background device monitoring
    void StopMonitoring();
    
    // Get current devices (thread-safe)
    std::vector<DeviceInfo> GetDevices() const;
    
    // Get selected device ID (thread-safe)
    std::string GetSelectedDeviceId() const;
    
    // Set selected device ID (thread-safe)
    void SetSelectedDeviceId(const std::string& deviceId);
    
    // Check if device is connected (thread-safe)
    bool IsDeviceConnected(const std::string& deviceId) const;
    
    // Clear all devices (thread-safe)
    void ClearDevices();
    
    // Check if monitoring is active
    bool IsMonitoring() const { return m_monitoring.load(); }
    
    // Execute ADB command and get output
    std::string ExecuteCommand(const std::string& command) const;
    
    // Destructor
    ~DeviceManager();

private:
    DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    
    // Background monitoring thread function
    void MonitoringThread();
    
    // Get device property
    std::string GetDeviceProperty(const std::string& deviceId, const std::string& property) const;
    
    // Get human-readable device name
    std::string GetHumanNameByID(const std::string& deviceId) const;
    
    // Update device list (called from background thread)
    void UpdateDeviceListInternal();
    
    // Member variables
    std::string m_adbPath;
    std::vector<DeviceInfo> m_devices;
    std::string m_selectedDeviceId;
    std::chrono::steady_clock::time_point m_lastDeviceUpdate;
    
    // Threading support
    mutable std::mutex m_mutex;
    std::thread m_monitoringThread;
    std::atomic<bool> m_monitoring{false};
};
