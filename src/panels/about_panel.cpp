#include "about_panel.h"
#include "core/registry/panel_registry.h"
#include "global_config.h"
#include "imgui.h"
#include <cstdio>
#include <string>

static std::string GetOSVersion() {
    std::string result;
    std::array<char, 128> buf;
#ifdef _WIN32
    FILE* pipe = _popen("ver", "r");
#else
    FILE* pipe = popen(
#  ifdef __APPLE__
        "sw_vers -productVersion",
#  else
        "uname -sr",
#  endif
        "r");
#endif
    if (!pipe) return "Unknown";
    while (fgets(buf.data(), buf.size(), pipe)) result += buf.data();
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result.empty() ? "Unknown" : result;
}

static bool s_show = false;
static std::string s_osVersion;

static void DrawAboutPanel() {
    if (!s_show) return;

    if (ImGui::Begin("About Seaweed", &s_show)) {
        char buf[1024];

        ImGui::Text("Name"); ImGui::SameLine(120);
        snprintf(buf, sizeof(buf), "Seaweed");
        ImGui::InputText("##name", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("Version"); ImGui::SameLine(120);
        snprintf(buf, sizeof(buf), "1.0.0");
        ImGui::InputText("##version", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("Developer"); ImGui::SameLine(120);
        snprintf(buf, sizeof(buf), "Raju Shingadiya");
        ImGui::InputText("##developer", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

        ImGui::Separator();

        ImGui::Text("OS"); ImGui::SameLine(120);
        snprintf(buf, sizeof(buf), "%s", s_osVersion.c_str());
        ImGui::InputText("##os", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("ADB Path"); ImGui::SameLine(120);
        std::string curAdbPath = GlobalConfig::GetADBPath();
        snprintf(buf, sizeof(buf), "%s", curAdbPath.empty() ? "Not found" : curAdbPath.c_str());
        ImGui::InputText("##adbpath", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("Device"); ImGui::SameLine(120);
        std::string selectedDevice = GlobalConfig::GetSelectedDeviceId();
        snprintf(buf, sizeof(buf), "%s", selectedDevice.empty() ? "None" : selectedDevice.c_str());
        ImGui::InputText("##device", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

        ImGui::Text("ADB Version"); ImGui::SameLine(120);
        std::string adbVerStr = "";
        std::string cmd = GlobalConfig::BuildAdbCommand("version");
        if (!cmd.empty()) {
            std::array<char, 128> verBuf;
            FILE* pipe = popen(cmd.c_str(), "r");
            if (pipe) {
                while (fgets(verBuf.data(), verBuf.size(), pipe))
                    adbVerStr += verBuf.data();
                pclose(pipe);
            }
        }
        if (adbVerStr.empty()) adbVerStr = "Not found";
        ImGui::InputTextMultiline("##adbver", &adbVerStr[0], adbVerStr.size() + 1,
            ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::End();
}

void RegisterAboutPanel() {
    s_osVersion = GetOSVersion();
    PanelRegistry::Get().Register({
        "about",
        "About Seaweed",
        "Seaweed",
        DrawAboutPanel,
        &s_show,
        {"app", "version", "info", "developer"}
    });
}
