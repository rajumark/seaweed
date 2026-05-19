#include "debug_info_panel.h"
#include "core/registry/panel_registry.h"
#include "global_config.h"
#include "device_manager.h"
#include "imgui.h"
#include <string>

static bool s_show = false;

static std::string GetAdbVersion() {
    std::string cmd = GlobalConfig::BuildAdbCommand("version");
    std::string out = DeviceManager::GetInstance().ExecuteCommand(cmd);
    if (out.empty()) return "Unknown";
    auto pos = out.find('\n');
    return out.substr(0, pos);
}

static void DrawDebugInfoPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Debug Info", &s_show)) {
        static std::string adbVersion;
        static std::string adbPath;
        static bool initialized = false;

        if (!initialized) {
            adbPath = GlobalConfig::GetADBPath();
            adbVersion = GetAdbVersion();
            initialized = true;
        }

        if (ImGui::BeginChild("DebugContent", ImVec2(0, 0), false)) {
            ImGui::SeparatorText("Environment");

            ImGui::Text("ADB Path");
            ImGui::SameLine();
            ImGui::TextDisabled(": %s", adbPath.empty() ? "Not configured" : adbPath.c_str());

            ImGui::Text("ADB Version");
            ImGui::SameLine();
            ImGui::TextDisabled(": %s", adbVersion.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void RegisterDebugInfoPanel() {
    PanelRegistry::Get().Register({
        "debug_info",
        "Debug Info",
        "Data",
        DrawDebugInfoPanel,
        &s_show,
        {"debug", "info", "about"}
    });
}
