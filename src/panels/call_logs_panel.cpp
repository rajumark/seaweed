#include "call_logs_panel.h"
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

struct CallLog {
    std::string id;
    std::string type;
    std::string typeLabel;
    std::string number;
    std::string name;
    std::string date;
    std::string dateFormatted;
    std::string duration;
    std::string durationFormatted;
    std::string geocodedLocation;
    std::map<std::string, std::string> rawdata;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::string CallTypeLabel(const std::string& val) {
    int t = 0;
    try { t = std::stoi(val); } catch (...) {}
    switch (t) {
        case 1: return "Incoming";
        case 2: return "Outgoing";
        case 3: return "Missed";
        case 4: return "Rejected";
        case 5: return "Blocked";
        case 6: return "Voicemail";
        default: return "Unknown";
    }
}

static ImVec4 CallTypeColor(const std::string& val) {
    int t = 0;
    try { t = std::stoi(val); } catch (...) {}
    switch (t) {
        case 1: return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        case 2: return ImVec4(0.0f, 0.8f, 0.8f, 1.0f);
        case 3: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        case 4: return ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
        case 5: return ImVec4(0.6f, 0.4f, 0.2f, 1.0f);
        case 6: return ImVec4(0.5f, 0.5f, 0.8f, 1.0f);
        default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

static std::string FormatDuration(const std::string& val) {
    long secs = 0;
    try { secs = std::stol(val); } catch (...) { return val; }
    if (secs < 0) return "0s";
    if (secs == 0) return "0s";
    long mins = secs / 60;
    secs = secs % 60;
    if (mins > 0)
        return std::to_string(mins) + "m " + std::to_string(secs) + "s";
    return std::to_string(secs) + "s";
}

static std::string FormatTimestamp(const std::string& val, std::string& relativeOut) {
    long long ts = 0;
    try { ts = std::stoll(val); } catch (...) { return val; }

    std::time_t t = ts / 1000;
    std::tm* tm = std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    std::string full(buf);

    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    long long diffMs = nowMs - ts;
    long long diffSec = diffMs / 1000;

    if (diffSec < 60) {
        relativeOut = "just now";
    } else if (diffSec < 3600) {
        relativeOut = std::to_string(diffSec / 60) + "m ago";
    } else if (diffSec < 86400) {
        relativeOut = std::to_string(diffSec / 3600) + "h ago";
    } else if (diffSec < 604800) {
        relativeOut = std::to_string(diffSec / 86400) + "d ago";
    } else {
        relativeOut = std::to_string(diffSec / 604800) + "w ago";
    }

    return full;
}

static std::map<std::string, std::string> ParseCallRow(const std::string& line) {
    std::map<std::string, std::string> row;

    auto pos = line.find("Row: ");
    if (pos == std::string::npos) return row;
    std::string rest = line.substr(pos + 5);
    auto commaPos = rest.find(", ");
    if (commaPos == std::string::npos) return row;
    rest = rest.substr(commaPos + 2);

    std::vector<std::string> tokens;
    size_t start = 0;
    while ((commaPos = rest.find(", ", start)) != std::string::npos) {
        tokens.push_back(rest.substr(start, commaPos - start));
        start = commaPos + 2;
    }
    tokens.push_back(rest.substr(start));

    for (size_t i = 0; i < tokens.size(); i++) {
        auto eq = tokens[i].find('=');
        if (eq == std::string::npos || eq == 0) continue;

        std::string key = tokens[i].substr(0, eq);
        std::string value = tokens[i].substr(eq + 1);

        while (i + 1 < tokens.size() && tokens[i + 1].find('=') == std::string::npos) {
            value += ", " + tokens[i + 1];
            i++;
        }

        key = Trim(key);
        value = Trim(value);

        if (!key.empty() && value != "NULL")
            row[key] = value;
    }

    return row;
}

static std::vector<CallLog> ParseCallLogs(const std::vector<std::string>& lines) {
    std::vector<CallLog> calls;

    for (const auto& line : lines) {
        auto raw = ParseCallRow(line);
        if (raw.empty()) continue;

        CallLog c;
        c.rawdata = raw;
        c.id = raw["_id"];
        c.type = raw["type"];
        c.typeLabel = CallTypeLabel(raw["type"]);
        c.number = raw["number"];
        c.name = raw["name"];
        c.duration = raw["duration"];
        c.durationFormatted = FormatDuration(raw["duration"]);
        c.geocodedLocation = raw["geocoded_location"];

        std::string relative;
        c.dateFormatted = FormatTimestamp(raw["date"], relative);
        c.date = relative;

        calls.push_back(std::move(c));
    }

    std::sort(calls.begin(), calls.end(), [](const CallLog& a, const CallLog& b) {
        int ai = 0, bi = 0;
        try { ai = std::stoi(a.id); } catch (...) {}
        try { bi = std::stoi(b.id); } catch (...) {}
        return ai > bi;
    });

    return calls;
}

class CallLogsPanel {
public:
    CallLogsPanel()
        : m_title("Call Logs")
        , m_capabilityId("list_call_logs")
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
            m_calls.clear();
        }

        auto capState = svc.GetState(m_capabilityId);
        if (m_phase == Phase::Loading && !capState.loading) {
            if (capState.success)
                m_calls = ParseCallLogs(capState.data);
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
        } else if (m_phase == Phase::Loading && m_calls.empty()) {
            ImGui::TextDisabled("Loading call logs...");
        } else if (m_calls.empty()) {
            ImGui::TextDisabled("No call logs found on device.");
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

        if (ImGui::BeginTable("CallLogTable", 3,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders,
            ImVec2(0, 0)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Number", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            std::string filterLower = m_filter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            for (size_t i = 0; i < m_calls.size(); i++) {
                const auto& call = m_calls[i];

                if (!m_filter.empty()) {
                    std::string haystack = call.number + " " + call.name + " " + call.typeLabel;
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

                ImVec4 col = CallTypeColor(call.type);
                std::string displayName = call.name.empty() ? "-" : call.name;

                ImGui::PushID(static_cast<int>(i));
                // Use Selectable in first cell spanning all columns for row selection
                ImGui::TableNextColumn();
                ImGui::Selectable("##sel", &isSelected,
                    ImGuiSelectableFlags_SpanAllColumns);
                if (ImGui::IsItemClicked() ||
                    (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                    m_selectedIndex = static_cast<int>(i);
                    m_detailCall = m_calls[i];
                    m_showDetailsPanel = true;
                }
                ImGui::SameLine();
                ImGui::TextColored(col, "%s", call.typeLabel.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", call.number.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", displayName.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_calls.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search calls...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Call Log Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImGui::TextColored(CallTypeColor(m_detailCall.type), "%s", m_detailCall.typeLabel.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(ID: %s)", m_detailCall.id.c_str());

        ImGui::SeparatorText("Call Info");
        detailField("Number", m_detailCall.number);
        detailField("Name", m_detailCall.name.empty() ? "-" : m_detailCall.name);
        detailField("Date", m_detailCall.dateFormatted);
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", m_detailCall.date.c_str());
        detailField("Duration", m_detailCall.durationFormatted);

        if (!m_detailCall.geocodedLocation.empty())
            detailField("Location", m_detailCall.geocodedLocation);

        if (!m_detailCall.rawdata.empty()) {
            ImGui::SeparatorText("Raw Data");

            std::vector<std::string> allKeys;
            for (const auto& [key, _] : m_detailCall.rawdata)
                allKeys.push_back(key);
            std::sort(allKeys.begin(), allKeys.end());

            if (ImGui::BeginTable("CallDataTable", 2,
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
                    auto it = m_detailCall.rawdata.find(key);
                    if (it != m_detailCall.rawdata.end())
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
    CallLog m_detailCall;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<CallLog> m_calls;

    std::string m_title;
    std::string m_capabilityId;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static CallLogsPanel* s_panel = nullptr;

static void DrawCallLogsPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterCallLogsPanel() {
    s_panel = new CallLogsPanel();
    PanelRegistry::Get().Register({
        "call_logs",
        "Call Logs",
        "Data",
        DrawCallLogsPanel,
        s_panel->GetShowPtr(),
        {"call logs", "calls", "phone", "history"}
    });
}
