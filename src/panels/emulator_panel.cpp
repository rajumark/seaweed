#include "emulator_panel.h"
#include "core/registry/panel_registry.h"
#include "emulator_manager.h"
#include "imgui.h"
#include <string>
#include <thread>
#include <cstdlib>

static bool s_show = false;
static bool s_hasAutoRefreshed = false;

static std::string GetHomeDir() {
#ifdef _WIN32
    const char* home = getenv("USERPROFILE");
    return home ? home : "C:\\";
#else
    const char* home = getenv("HOME");
    return home ? home : "/tmp";
#endif
}

static void OpenInFileManager(const std::string& path) {
    std::string cmd;
#ifdef _WIN32
    cmd = "explorer \"" + path + "\"";
#elif defined(__APPLE__)
    cmd = "open \"" + path + "\"";
#else
    cmd = "xdg-open \"" + path + "\"";
#endif
    std::thread([cmd]() { system(cmd.c_str()); }).detach();
}

static std::string GetAvdFolderPath(const std::string& avdName) {
    const char* avdHome = getenv("ANDROID_AVD_HOME");
    if (avdHome && avdHome[0])
        return std::string(avdHome) + "/" + avdName + ".avd/";
    return GetHomeDir() + "/.android/avd/" + avdName + ".avd/";
}

static void DrawEmulatorPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Emulator", &s_show)) {
        auto& em = EmulatorManager::GetInstance();
        bool scanning = em.IsScanning();

        // Auto-refresh on first show
        if (!s_hasAutoRefreshed && !scanning) {
            em.ScanEmulators();
            s_hasAutoRefreshed = true;
            scanning = true;
        }

        if (ImGui::Button(scanning ? "Scanning..." : "Refresh", ImVec2(120, 0))) {
            if (!scanning) {
                em.ScanEmulators();
                s_hasAutoRefreshed = true;
            }
        }

        ImGui::Separator();

        auto emulators = em.GetEmulators();
        if (emulators.empty() && !scanning) {
            ImGui::TextDisabled("No emulators found. Press Refresh to scan.");
        } else if (scanning) {
            ImGui::TextDisabled("Scanning for emulators...");
        } else {
            if (ImGui::BeginTable("emulators", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Device ID");
                ImGui::TableSetupColumn("Actions");
                ImGui::TableSetupColumn("Files");
                ImGui::TableHeadersRow();

                for (int i = 0; i < (int)emulators.size(); i++) {
                    const auto& e = emulators[i];

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", e.name.c_str());
                    ImGui::TableNextColumn();
                    if (e.isRunning) {
                        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Running");
                    } else {
                        ImGui::Text("Stopped");
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", e.isRunning ? e.deviceId.c_str() : "-");
                    ImGui::TableNextColumn();

                    ImGui::PushID(i);
                    if (e.isRunning) {
                        if (ImGui::Button("Stop", ImVec2(60, 0))) {
                            em.StopEmulator(e.deviceId);
                        }
                    } else {
                        if (ImGui::Button("Start", ImVec2(60, 0))) {
                            em.StartEmulator(e.name);
                        }
                    }
                    ImGui::PopID();

                    ImGui::TableNextColumn();
                    ImGui::PushID(1000 + i);
                    if (ImGui::Button("Open Folder", ImVec2(0, 0))) {
                        std::string folder = GetAvdFolderPath(e.name);
                        OpenInFileManager(folder);
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

void RegisterEmulatorPanel() {
    PanelRegistry::Get().Register({
        "emulator",
        "Emulator",
        "Devices",
        DrawEmulatorPanel,
        &s_show,
        {}
    });
}
