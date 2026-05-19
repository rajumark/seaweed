#include "lifecycle_panel.h"
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
#include <ctime>

struct LifecycleEntry {
    std::string time;
    std::string type;
    std::string packageName;
    std::string className;
    std::map<std::string, std::string> rawdata;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::string LifecycleTypeLabel(const std::string& val) {
    int t = 0;
    try { t = std::stoi(val); } catch (...) { return val; }
    switch (t) {
        case 1: return "Activity Resumed";
        case 2: return "Activity Paused";
        case 3: return "Activity Stopped";
        case 4: return "Activity Destroyed";
        case 5: return "Foreground Service Start";
        case 6: return "Foreground Service Stop";
        case 7: return "Config Changed";
        case 8: return "Activity Created";
        case 9: return "Activity Started";
        case 10: return "Saved Instance State";
        default: return "Type " + val;
    }
}

static ImVec4 LifecycleTypeColor(const std::string& val) {
    int t = 0;
    try { t = std::stoi(val); } catch (...) {}
    switch (t) {
        case 1: return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        case 2: return ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
        case 3: return ImVec4(0.8f, 0.4f, 0.0f, 1.0f);
        case 4: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        case 5: return ImVec4(0.3f, 0.6f, 1.0f, 1.0f);
        case 6: return ImVec4(0.6f, 0.4f, 0.8f, 1.0f);
        case 7: return ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
        case 8: return ImVec4(0.3f, 0.9f, 0.6f, 1.0f);
        case 9: return ImVec4(0.4f, 0.7f, 0.4f, 1.0f);
        case 10: return ImVec4(0.7f, 0.5f, 0.3f, 1.0f);
        default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

static std::map<std::string, std::string> ParseLifecycleLine(const std::string& line) {
    std::map<std::string, std::string> row;

    std::vector<std::string> tokens;
    std::string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            cur += c;
        } else if (c == ' ' && !inQuotes) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) tokens.push_back(cur);

    for (const auto& tok : tokens) {
        auto eq = tok.find('=');
        if (eq == std::string::npos || eq == 0) continue;
        std::string key = tok.substr(0, eq);
        std::string value = tok.substr(eq + 1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        key = Trim(key);
        value = Trim(value);
        if (!key.empty() && value != "NULL")
            row[key] = value;
    }

    return row;
}

static std::vector<LifecycleEntry> ParseLifecycleEvents(const std::vector<std::string>& lines) {
    std::vector<LifecycleEntry> entries;

    for (const auto& line : lines) {
        auto raw = ParseLifecycleLine(line);
        if (raw.empty() || raw.find("type") == raw.end())
            continue;

        LifecycleEntry e;
        e.rawdata = raw;

        auto it = raw.find("time");
        if (it != raw.end()) e.time = it->second;

        it = raw.find("type");
        if (it != raw.end()) e.type = it->second;

        it = raw.find("package");
        if (it != raw.end()) e.packageName = it->second;

        it = raw.find("class");
        if (it != raw.end()) e.className = it->second;

        entries.push_back(std::move(e));
    }

    std::sort(entries.begin(), entries.end(), [](const LifecycleEntry& a, const LifecycleEntry& b) {
        return a.time > b.time;
    });

    return entries;
}

class LifecyclePanel {
public:
    LifecyclePanel()
        : m_title("Lifecycle")
        , m_capabilityId("list_lifecycle")
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
            m_entries.clear();
        }

        auto capState = svc.GetState(m_capabilityId);
        if (m_phase == Phase::Loading && !capState.loading) {
            if (capState.success)
                m_entries = ParseLifecycleEvents(capState.data);
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
        } else if (m_phase == Phase::Loading && m_entries.empty()) {
            ImGui::TextDisabled("Loading lifecycle events...");
        } else if (m_entries.empty()) {
            ImGui::TextDisabled("No lifecycle events found on device.");
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

        if (ImGui::BeginTable("LifecycleTable", 4,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders,
            ImVec2(0, 0)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 180);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Package", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            std::string filterLower = m_filter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            for (size_t i = 0; i < m_entries.size(); i++) {
                const auto& entry = m_entries[i];

                if (!m_filter.empty()) {
                    std::string haystack = entry.time + " " +
                        LifecycleTypeLabel(entry.type) + " " +
                        entry.packageName + " " + entry.className;
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

                ImVec4 col = LifecycleTypeColor(entry.type);

                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextColumn();
                ImGui::Selectable("##sel", &isSelected,
                    ImGuiSelectableFlags_SpanAllColumns);
                if (ImGui::IsItemClicked() ||
                    (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                    m_selectedIndex = static_cast<int>(i);
                    m_detailEntry = m_entries[i];
                    m_showDetailsPanel = true;
                }
                ImGui::SameLine();
                ImGui::Text("%s", entry.time.c_str());

                ImGui::TableNextColumn();
                ImGui::TextColored(col, "%s", LifecycleTypeLabel(entry.type).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.packageName.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.className.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_entries.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search lifecycle...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Lifecycle Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImGui::TextColored(LifecycleTypeColor(m_detailEntry.type), "%s",
            LifecycleTypeLabel(m_detailEntry.type).c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(Time: %s)", m_detailEntry.time.c_str());

        ImGui::SeparatorText("Event Info");
        detailField("Time", m_detailEntry.time);
        detailField("Type", LifecycleTypeLabel(m_detailEntry.type));
        detailField("Package", m_detailEntry.packageName.empty() ? "-" : m_detailEntry.packageName);
        detailField("Class", m_detailEntry.className.empty() ? "-" : m_detailEntry.className);

        auto it = m_detailEntry.rawdata.find("instanceId");
        if (it != m_detailEntry.rawdata.end())
            detailField("Instance ID", it->second);
        it = m_detailEntry.rawdata.find("flags");
        if (it != m_detailEntry.rawdata.end())
            detailField("Flags", it->second);

        if (!m_detailEntry.rawdata.empty()) {
            ImGui::SeparatorText("Raw Data");

            std::vector<std::string> allKeys;
            for (const auto& [key, _] : m_detailEntry.rawdata)
                allKeys.push_back(key);
            std::sort(allKeys.begin(), allKeys.end());

            if (ImGui::BeginTable("LifecycleDataTable", 2,
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
                    auto it2 = m_detailEntry.rawdata.find(key);
                    if (it2 != m_detailEntry.rawdata.end())
                        ImGui::TextWrapped("%s", it2->second.c_str());
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
    LifecycleEntry m_detailEntry;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<LifecycleEntry> m_entries;

    std::string m_title;
    std::string m_capabilityId;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static LifecyclePanel* s_panel = nullptr;

static void DrawLifecyclePanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterLifecyclePanel() {
    s_panel = new LifecyclePanel();
    PanelRegistry::Get().Register({
        "lifecycle",
        "Lifecycle",
        "Data",
        DrawLifecyclePanel,
        s_panel->GetShowPtr(),
        {"lifecycle", "usage", "stats"}
    });
}
