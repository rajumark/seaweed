#include "processes_panel.h"
#include "core/registry/panel_registry.h"
#include "core/capability/capability_service.h"
#include "global_config.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <chrono>

struct ProcessInfo {
    std::string pid;
    std::string name;
    std::map<std::string, std::string> rawdata;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static ProcessInfo ParseProcessLine(const std::string& line) {
    ProcessInfo info;

    auto pos = line.find("app=ProcessRecord{");
    if (pos == std::string::npos) return info;

    auto braceStart = pos + 19;
    auto braceEnd = line.find('}', braceStart);
    if (braceEnd == std::string::npos) return info;

    std::string content = line.substr(braceStart, braceEnd - braceStart);

    auto spacePos = content.find(' ');
    if (spacePos == std::string::npos) return info;

    std::string rest = content.substr(spacePos + 1);

    auto colonPos = rest.find(':');
    if (colonPos == std::string::npos) return info;

    info.pid = rest.substr(0, colonPos);

    std::string nameAndUid = rest.substr(colonPos + 1);
    auto slashPos = nameAndUid.rfind('/');
    if (slashPos != std::string::npos) {
        info.name = nameAndUid.substr(0, slashPos);
        info.rawdata["uid"] = nameAndUid.substr(slashPos + 1);
    } else {
        info.name = nameAndUid;
    }

    info.rawdata["pid"] = info.pid;
    info.rawdata["name"] = info.name;
    info.rawdata["raw"] = content;

    return info;
}

static std::vector<ProcessInfo> ParseProcesses(const std::vector<std::string>& lines) {
    std::map<std::string, ProcessInfo> pidMap;

    for (const auto& line : lines) {
        auto info = ParseProcessLine(line);
        if (info.pid.empty()) continue;
        if (pidMap.find(info.pid) != pidMap.end()) continue;
        pidMap[info.pid] = std::move(info);
    }

    std::vector<ProcessInfo> procs;
    for (auto& kv : pidMap)
        procs.push_back(std::move(kv.second));

    std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        int ai = 0, bi = 0;
        try { ai = std::stoi(a.pid); } catch (...) {}
        try { bi = std::stoi(b.pid); } catch (...) {}
        return ai < bi;
    });

    return procs;
}

class ProcessesPanel {
public:
    ProcessesPanel()
        : m_title("Processes")
        , m_capabilityId("list_processes")
        , m_refreshMs(5000)
    {}

    void Draw() {
        if (!m_show) return;
        if (!ImGui::Begin(m_title.c_str(), &m_show)) {
            ImGui::End();
            return;
        }

        auto& svc = CapabilityService::Get();
        auto now = std::chrono::steady_clock::now();

        std::string currentDevice = GlobalConfig::GetSelectedDeviceId();
        if (currentDevice != m_cachedDeviceId) {
            m_cachedDeviceId = currentDevice;
            svc.InvalidateCache(m_capabilityId);
            m_phase = Phase::Idle;
            m_processes.clear();
        }

        auto capState = svc.GetState(m_capabilityId);
        if (m_phase == Phase::Loading && !capState.loading) {
            if (capState.success)
                m_processes = ParseProcesses(capState.data);
            m_phase = Phase::WithData;
        }

        if (m_phase == Phase::Idle && !m_cachedDeviceId.empty()) {
            svc.Execute(m_capabilityId, {m_cachedDeviceId});
            m_phase = Phase::Loading;
            m_lastRefresh = now;
        }

        if (m_phase != Phase::Loading && !m_cachedDeviceId.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_lastRefresh).count();
            if (elapsed >= m_refreshMs) {
                svc.Execute(m_capabilityId, {m_cachedDeviceId});
                m_phase = Phase::Loading;
                m_lastRefresh = now;
            }
        }

        if (m_cachedDeviceId.empty()) {
            ImGui::TextDisabled("No device selected. Connect a device first.");
        } else if (m_phase == Phase::Loading && m_processes.empty()) {
            ImGui::TextDisabled("Loading processes...");
        } else if (m_processes.empty()) {
            ImGui::TextDisabled("No processes found on device.");
        } else {
            RenderContent();
        }

        ImGui::End();

        RenderDetailWindow();
    }

    bool* GetShowPtr() { return &m_show; }

private:
    enum class Phase { Idle, Loading, WithData };

    void RenderContent() {
        RenderToolbar();

        ImGui::Separator();

        if (ImGui::BeginTable("ProcessTable", 2,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders,
            ImVec2(0, 0)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            std::string filterLower = m_filter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            for (size_t i = 0; i < m_processes.size(); i++) {
                const auto& proc = m_processes[i];

                if (!m_filter.empty()) {
                    std::string haystack = proc.pid + " " + proc.name;
                    std::transform(haystack.begin(), haystack.end(), haystack.begin(),
                                   [](unsigned char c) { return std::tolower(c); });
                    if (haystack.find(filterLower) == std::string::npos)
                        continue;
                }

                ImGui::TableNextRow();
                bool isSelected = (m_selectedIndex == static_cast<int>(i));

                if (isSelected)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                        IM_COL32(60, 100, 180, 80));

                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextColumn();
                ImGui::Selectable("##sel", &isSelected,
                    ImGuiSelectableFlags_SpanAllColumns);
                if (ImGui::IsItemClicked() ||
                    (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                    m_selectedIndex = static_cast<int>(i);
                    m_detailProc = m_processes[i];
                    m_showDetailsPanel = true;
                }
                ImGui::SameLine();
                ImGui::Text("%s", proc.pid.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", proc.name.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_processes.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search processes...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Process Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImGui::Text("PID: %s", m_detailProc.pid.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", m_detailProc.name.c_str());

        ImGui::SeparatorText("Process Info");
        detailField("PID", m_detailProc.pid);
        detailField("Name", m_detailProc.name);

        if (!m_detailProc.rawdata.empty()) {
            ImGui::SeparatorText("Raw Data");

            std::vector<std::string> allKeys;
            for (const auto& [key, _] : m_detailProc.rawdata)
                allKeys.push_back(key);
            std::sort(allKeys.begin(), allKeys.end());

            if (ImGui::BeginTable("ProcDataTable", 2,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Field");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                for (const auto& key : allKeys) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", key.c_str());
                    ImGui::TableNextColumn();
                    auto it = m_detailProc.rawdata.find(key);
                    if (it != m_detailProc.rawdata.end())
                        ImGui::TextWrapped("%s", it->second.c_str());
                }
                ImGui::EndTable();
            }
        }

        ImGui::End();
    }

    void detailField(const char* label, const std::string& value) {
        ImGui::Text("%s: ", label);
        ImGui::SameLine();
        ImGui::TextWrapped("%s", value.c_str());
    }

    void TriggerRefresh() {
        CapabilityService::Get().InvalidateCache(m_capabilityId);
        m_phase = Phase::Idle;
    }

    bool m_show = false;
    bool m_showDetailsPanel = false;
    ProcessInfo m_detailProc;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<ProcessInfo> m_processes;

    std::string m_title;
    std::string m_capabilityId;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static ProcessesPanel* s_panel = nullptr;

static void DrawProcessesPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterProcessesPanel() {
    s_panel = new ProcessesPanel();
    PanelRegistry::Get().Register({
        "processes",
        "Processes",
        "Data",
        DrawProcessesPanel,
        s_panel->GetShowPtr(),
        {"processes", "services", "running"}
    });
}
