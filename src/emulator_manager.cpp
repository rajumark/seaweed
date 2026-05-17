#include "emulator_manager.h"
#include <iostream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdio>
#include <thread>
#include <cstdlib>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

EmulatorManager& EmulatorManager::GetInstance() {
    static EmulatorManager instance;
    return instance;
}

void EmulatorManager::SetAdbPath(const std::string& path) {
    m_adbPath = path;
}

bool EmulatorManager::IsScanning() const {
    return m_scanning.load();
}

std::string EmulatorManager::GetEmulatorPath() const {
    return m_emulatorBinary;
}

std::vector<std::string> EmulatorManager::GetDefaultEmulatorPaths() {
#ifdef _WIN32
    return {
        "%USERPROFILE%\\AppData\\Local\\Android\\Sdk\\emulator",
        "C:\\Users\\%USERNAME%\\AppData\\Local\\Android\\Sdk\\emulator",
        "C:\\Program Files\\Android\\Android Studio\\bin"
    };
#elif defined(__APPLE__)
    return {
        "~$USER/Library/Android/sdk/emulator",
        "/Applications/Android Studio.app/Contents/Developer/Sdk/emulator"
    };
#else
    return {
        "~$USER/Android/Sdk/emulator",
        "/usr/lib/android-sdk/emulator",
        "/opt/android-sdk/emulator"
    };
#endif
}

std::string EmulatorManager::ExpandHome(const std::string& path) const {
    std::string result = path;

    auto expandVar = [&](const std::string& var, const std::string& val) {
        size_t pos = 0;
        while ((pos = result.find(var, pos)) != std::string::npos) {
            result.replace(pos, var.length(), val);
            pos += val.length();
        }
    };

#ifdef _WIN32
    const char* userProfile = getenv("USERPROFILE");
    if (userProfile) expandVar("%USERPROFILE%", userProfile);
    const char* userName = getenv("USERNAME");
    if (userName) expandVar("%USERNAME%", userName);
#else
    const char* user = getenv("USER");
    if (!user) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) user = pw->pw_name;
    }
    if (user) {
        expandVar("$USER", user);
        const char* home = getenv("HOME");
        if (home) expandVar(std::string("~") + user, std::string(home));
    }
#endif

    return result;
}

std::string EmulatorManager::ExecuteCommand(const std::string& cmd) const {
    std::string result;
    std::array<char, 128> buffer;
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
    auto closer = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(closer)> pipe(popen(cmd.c_str(), "r"), closer);
#endif
    if (!pipe) return result;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();
    return result;
}

std::vector<std::string> EmulatorManager::ParseLines(const std::string& output) const {
    std::vector<std::string> lines;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();
            if (!line.empty())
                lines.push_back(line);
        }
    }
    return lines;
}

std::string EmulatorManager::FindEmulatorBinary() const {
    if (!m_emulatorBinary.empty())
        return m_emulatorBinary;

    std::string binaryName =
#ifdef _WIN32
        "emulator.exe";
#else
        "emulator";
#endif

    for (const auto& rawPath : GetDefaultEmulatorPaths()) {
        std::string expanded = ExpandHome(rawPath);
        std::string candidate = expanded + "/" + binaryName;
        FILE* f = fopen(candidate.c_str(), "r");
        if (f) {
            fclose(f);
            return candidate;
        }
    }
    return "";
}

bool EmulatorManager::ScanEmulators() {
    if (m_scanning.exchange(true)) return false;

    m_emulatorBinary = FindEmulatorBinary();
    std::vector<EmulatorInfo> results;

    if (m_emulatorBinary.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_emulators = results;
        m_scanning.store(false);
        return true;
    }

    std::vector<std::string> avds;
    std::string listCmd = "\"" + m_emulatorBinary + "\" -list-avds 2>/dev/null";
    std::string listOutput = ExecuteCommand(listCmd);
    for (auto& line : ParseLines(listOutput)) {
        if (!line.empty() && line.find(" ") == std::string::npos)
            avds.push_back(line);
    }

    for (const auto& avd : avds) {
        EmulatorInfo info;
        info.name = avd;
        info.isRunning = false;
        results.push_back(info);
    }

    if (!m_adbPath.empty()) {
        std::string adbCmd = "\"" + m_adbPath + "\" devices";
        std::string adbOutput = ExecuteCommand(adbCmd);
        auto lines = ParseLines(adbOutput);

        for (const auto& line : lines) {
            if (line.find("List of devices") != std::string::npos ||
                line.find("* daemon") != std::string::npos)
                continue;

            std::istringstream iss(line);
            std::string deviceId, state;
            if (!(iss >> deviceId >> state)) continue;
            if (deviceId.find("emulator-") != 0) continue;

            std::string avdName;
            std::string propCmd = "\"" + m_adbPath + "\" -s " + deviceId +
                " shell getprop ro.kernel.qemu.avd_name 2>/dev/null";
            std::string propOutput = ExecuteCommand(propCmd);
            for (auto& p : ParseLines(propOutput)) {
                if (!p.empty() && p.find("WARNING") == std::string::npos &&
                    p.find("error") == std::string::npos) {
                    avdName = p;
                    break;
                }
            }

            if (!avdName.empty()) {
                bool found = false;
                for (auto& r : results) {
                    if (r.name == avdName) {
                        r.isRunning = true;
                        r.deviceId = deviceId;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    EmulatorInfo info;
                    info.name = avdName;
                    info.deviceId = deviceId;
                    info.isRunning = true;
                    results.push_back(info);
                }
            } else {
                EmulatorInfo info;
                info.name = deviceId;
                info.deviceId = deviceId;
                info.isRunning = true;
                results.push_back(info);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::stable_partition(results.begin(), results.end(),
            [](const EmulatorInfo& e) { return e.isRunning; });
        m_emulators = results;
    }
    m_scanning.store(false);
    return true;
}

std::vector<EmulatorInfo> EmulatorManager::GetEmulators() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_emulators;
}

void EmulatorManager::StartEmulator(const std::string& avdName) {
    if (m_emulatorBinary.empty()) return;
    std::string cmd = "\"" + m_emulatorBinary + "\" -avd " + avdName;
#ifdef _WIN32
    cmd = "start \"\" " + cmd;
#else
    cmd += " &";
#endif
    std::thread([cmd]() {
        system(cmd.c_str());
    }).detach();
}

void EmulatorManager::StopEmulator(const std::string& deviceId) {
    if (m_adbPath.empty()) return;
    std::string cmd = "\"" + m_adbPath + "\" -s " + deviceId + " emu kill";
    ExecuteCommand(cmd);
}
