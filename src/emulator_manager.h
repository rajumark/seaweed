#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>

struct EmulatorInfo {
    std::string name;
    std::string deviceId;
    bool isRunning;
};

class EmulatorManager {
public:
    static EmulatorManager& GetInstance();

    void SetAdbPath(const std::string& path);
    bool ScanEmulators();
    std::vector<EmulatorInfo> GetEmulators() const;
    void StartEmulator(const std::string& avdName);
    void StopEmulator(const std::string& deviceId);
    bool IsScanning() const;
    std::string GetEmulatorPath() const;

private:
    EmulatorManager() = default;
    EmulatorManager(const EmulatorManager&) = delete;
    EmulatorManager& operator=(const EmulatorManager&) = delete;

    static std::vector<std::string> GetDefaultEmulatorPaths();
    std::string ExpandHome(const std::string& path) const;
    std::string ExecuteCommand(const std::string& cmd) const;
    std::vector<std::string> ParseLines(const std::string& output) const;
    std::string FindEmulatorBinary() const;

    std::string m_adbPath;
    std::string m_emulatorBinary;
    std::vector<EmulatorInfo> m_emulators;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_scanning{false};
};
