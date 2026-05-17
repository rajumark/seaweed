#include "device_manager.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <array>
#include <memory>
#include <cstdio>
#include <algorithm>
#include <thread>

DeviceManager& DeviceManager::GetInstance() {
    static DeviceManager instance;
    return instance;
}

void DeviceManager::Initialize(const std::string& adbPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_adbPath = adbPath;
    m_lastDeviceUpdate = std::chrono::steady_clock::now();
}

void DeviceManager::StartMonitoring() {
    if (m_monitoring.load()) {
        return; // Already monitoring
    }
    
    m_monitoring.store(true);
    m_monitoringThread = std::thread(&DeviceManager::MonitoringThread, this);
}

void DeviceManager::StopMonitoring() {
    if (!m_monitoring.load()) {
        return; // Not monitoring
    }
    
    m_monitoring.store(false);
    
    if (m_monitoringThread.joinable()) {
        m_monitoringThread.join();
    }
}

std::vector<DeviceInfo> DeviceManager::GetDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices;
}

std::string DeviceManager::GetSelectedDeviceId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_selectedDeviceId;
}

void DeviceManager::SetSelectedDeviceId(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_selectedDeviceId = deviceId;
}

bool DeviceManager::IsDeviceConnected(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& device : m_devices) {
        if (device.deviceId == deviceId) {
            return device.isConnected;
        }
    }
    return false;
}

void DeviceManager::ClearDevices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices.clear();
    m_selectedDeviceId.clear();
}

DeviceManager::~DeviceManager() {
    StopMonitoring();
}

void DeviceManager::MonitoringThread() {
    while (m_monitoring.load()) {
        try {
            UpdateDeviceListInternal();
            
            // Sleep for 2 seconds before next update using a different approach
            for (int i = 0; i < 20 && m_monitoring.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in monitoring thread: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown error in monitoring thread" << std::endl;
        }
    }
}

std::string DeviceManager::ExecuteCommand(const std::string& command) const {
    std::string result;
    std::array<char, 128> buffer;
    
    // Use _popen on Windows, popen on Unix-like systems
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    auto pipeCloser = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipeCloser)> pipe(popen(command.c_str(), "r"), pipeCloser);
#endif
    
    if (!pipe) {
        return "";
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

std::string DeviceManager::GetDeviceProperty(const std::string& deviceId, const std::string& property) const {
    try {
        std::string command = "\"" + m_adbPath + "\" -s " + deviceId + " shell getprop " + property;
        std::string output = ExecuteCommand(command);
        
        // Clean up the output (remove newlines and extra spaces)
        if (!output.empty()) {
            output.erase(std::remove(output.begin(), output.end(), '\n'), output.end());
            output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());
            
            // Trim leading/trailing spaces
            size_t start = output.find_first_not_of(" \t");
            if (start != std::string::npos) {
                size_t end = output.find_last_not_of(" \t");
                if (end != std::string::npos) {
                    output = output.substr(start, end - start + 1);
                } else {
                    output = output.substr(start);
                }
            } else {
                output.clear();
            }
            
            // Check if the output is "null" or empty
            if (output == "null" || output.empty()) {
                return "";
            }
            
            return output;
        }
    } catch (...) {
        // If property retrieval fails, return empty string
    }
    return "";
}

std::string DeviceManager::GetHumanNameByID(const std::string& deviceId) const {
    // If device ID is empty, return empty string
    if (deviceId.empty()) {
        return "";
    }
    
    std::string name;
    
    // Check if device is an emulator
    if (deviceId.find("emulator") != std::string::npos) {
        // For emulators: get avd_name and version.release
        std::string avdName = GetDeviceProperty(deviceId, "ro.kernel.qemu.avd_name");
        std::string versionRelease = GetDeviceProperty(deviceId, "ro.build.version.release");
        
        if (!avdName.empty() && !versionRelease.empty()) {
            name = avdName + " " + versionRelease;
        } else if (!avdName.empty()) {
            name = avdName;
        } else if (!versionRelease.empty()) {
            name = versionRelease;
        }
    } else {
        // For real devices: get brand, model, and version.release
        std::string brand = GetDeviceProperty(deviceId, "ro.product.brand");
        std::string model = GetDeviceProperty(deviceId, "ro.product.model");
        std::string versionRelease = GetDeviceProperty(deviceId, "ro.build.version.release");
        
        if (!brand.empty() && !model.empty() && !versionRelease.empty()) {
            name = brand + " " + model + " " + versionRelease;
        } else if (!brand.empty() && !model.empty()) {
            name = brand + " " + model;
        } else if (!model.empty() && !versionRelease.empty()) {
            name = model + " " + versionRelease;
        } else if (!brand.empty() && !versionRelease.empty()) {
            name = brand + " " + versionRelease;
        } else if (!brand.empty()) {
            name = brand;
        } else if (!model.empty()) {
            name = model;
        } else if (!versionRelease.empty()) {
            name = versionRelease;
        }
    }
    
    // If name is empty, contains "null", or is invalid, return device ID as fallback
    if (name.empty() || name.find("null") != std::string::npos) {
        return deviceId;
    }
    
    return name;
}

void DeviceManager::UpdateDeviceListInternal() {
    try {
        if (m_adbPath.empty()) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_devices.clear();
            return;
        }

        std::string command = "\"" + m_adbPath + "\" devices";
        std::string output = ExecuteCommand(command);

        std::vector<DeviceInfo> newDevices;
        std::istringstream iss(output);
        std::string line;

        std::getline(iss, line);

        while (std::getline(iss, line)) {
            if (line.empty()) continue;

            std::istringstream lineStream(line);
            std::string deviceId, stateStr;
            if (!(lineStream >> deviceId >> stateStr)) continue;

            AdbDeviceState state = ParseAdbState(stateStr);
            std::string deviceName;
            std::string osVersion;

            if (state == AdbDeviceState::Device) {
                deviceName = GetHumanNameByID(deviceId);
                osVersion = GetDeviceProperty(deviceId, "ro.build.version.release");
            }

            newDevices.emplace_back(deviceId, deviceName, stateStr, state, osVersion);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_devices = newDevices;

            if (!m_selectedDeviceId.empty()) {
                bool found = false;
                for (const auto& d : m_devices) {
                    if (d.deviceId == m_selectedDeviceId) {
                        found = true;
                        break;
                    }
                }
                if (!found) m_selectedDeviceId.clear();
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error updating device list: " << e.what() << std::endl;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_devices.clear();
    } catch (...) {
        std::cerr << "Unknown error updating device list" << std::endl;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_devices.clear();
    }
}
