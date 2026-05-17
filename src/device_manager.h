#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>

enum class AdbDeviceState {
    Unknown,
    Device,
    Offline,
    Unauthorized,
    Bootloader,
    Recovery,
    Sideload,
    Rescue,
    NoPermissions,
    Host,
    Connecting
};

inline AdbDeviceState ParseAdbState(std::string_view state) {
    if (state == "device")         return AdbDeviceState::Device;
    if (state == "offline")        return AdbDeviceState::Offline;
    if (state == "unauthorized")   return AdbDeviceState::Unauthorized;
    if (state == "bootloader")     return AdbDeviceState::Bootloader;
    if (state == "recovery")       return AdbDeviceState::Recovery;
    if (state == "sideload")       return AdbDeviceState::Sideload;
    if (state == "rescue")         return AdbDeviceState::Rescue;
    if (state == "no permissions") return AdbDeviceState::NoPermissions;
    if (state == "host")           return AdbDeviceState::Host;
    if (state == "connecting")     return AdbDeviceState::Connecting;
    return AdbDeviceState::Unknown;
}

inline const char* AdbStateToString(AdbDeviceState state) {
    switch (state) {
        case AdbDeviceState::Device:        return "Device";
        case AdbDeviceState::Offline:       return "Offline";
        case AdbDeviceState::Unauthorized:  return "Unauthorized";
        case AdbDeviceState::Bootloader:    return "Bootloader";
        case AdbDeviceState::Recovery:      return "Recovery";
        case AdbDeviceState::Sideload:      return "Sideload";
        case AdbDeviceState::Rescue:        return "Rescue";
        case AdbDeviceState::NoPermissions: return "No Permissions";
        case AdbDeviceState::Host:          return "Host";
        case AdbDeviceState::Connecting:    return "Connecting";
        default:                            return "Unknown";
    }
}

struct DeviceInfo {
    std::string deviceId;
    std::string deviceName;
    std::string status;
    bool isConnected;
    AdbDeviceState state = AdbDeviceState::Unknown;
    std::string osVersion;

    DeviceInfo() : isConnected(false), state(AdbDeviceState::Unknown) {}
    DeviceInfo(const std::string& id, const std::string& name, const std::string& stat,
               AdbDeviceState devState = AdbDeviceState::Unknown,
               const std::string& osVer = "")
        : deviceId(id), deviceName(name), status(stat), isConnected(true), state(devState), osVersion(osVer) {}
};

class DeviceManager {
public:
    static DeviceManager& GetInstance();

    void Initialize(const std::string& adbPath);

    void StartMonitoring();

    void StopMonitoring();

    std::vector<DeviceInfo> GetDevices() const;

    std::string GetSelectedDeviceId() const;

    void SetSelectedDeviceId(const std::string& deviceId);

    bool IsDeviceConnected(const std::string& deviceId) const;

    void ClearDevices();

    bool IsMonitoring() const { return m_monitoring.load(); }

    std::string ExecuteCommand(const std::string& command) const;

    ~DeviceManager();

private:
    DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    void MonitoringThread();

    std::string GetDeviceProperty(const std::string& deviceId, const std::string& property) const;

    std::string GetHumanNameByID(const std::string& deviceId) const;

    void UpdateDeviceListInternal();

    std::string m_adbPath;
    std::vector<DeviceInfo> m_devices;
    std::string m_selectedDeviceId;
    std::chrono::steady_clock::time_point m_lastDeviceUpdate;

    mutable std::mutex m_mutex;
    std::thread m_monitoringThread;
    std::atomic<bool> m_monitoring{false};
};
