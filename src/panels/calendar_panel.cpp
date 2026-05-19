#include "calendar_panel.h"
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

struct CalendarEvent {
    std::string id;
    std::string title;
    std::string dtstart;
    std::string dtstartFormatted;
    std::string dtend;
    std::string dtendFormatted;
    std::string description;
    std::string eventLocation;
    std::string allDay;
    std::string duration;
    std::string calendar_displayName;
    std::map<std::string, std::string> rawdata;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::string FormatTimestamp(const std::string& val) {
    long long ts = 0;
    try { ts = std::stoll(val); } catch (...) { return val; }
    std::time_t t = ts / 1000;
    std::tm* tm = std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buf);
}

static std::map<std::string, std::string> ParseCalendarRow(const std::string& line) {
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

static std::vector<CalendarEvent> ParseCalendarEvents(const std::vector<std::string>& lines) {
    std::vector<CalendarEvent> events;

    for (const auto& line : lines) {
        auto raw = ParseCalendarRow(line);
        if (raw.empty()) continue;

        CalendarEvent e;
        e.rawdata = raw;
        e.id = raw["_id"];
        e.title = raw["title"];
        e.dtstart = raw["dtstart"];
        e.dtstartFormatted = FormatTimestamp(raw["dtstart"]);
        e.dtend = raw["dtend"];
        e.dtendFormatted = FormatTimestamp(raw["dtend"]);
        e.description = raw["description"];
        e.eventLocation = raw["eventLocation"];
        e.allDay = raw["allDay"];
        e.duration = raw["duration"];
        e.calendar_displayName = raw["calendar_displayName"];

        events.push_back(std::move(e));
    }

    std::sort(events.begin(), events.end(), [](const CalendarEvent& a, const CalendarEvent& b) {
        long long ai = 0, bi = 0;
        try { ai = std::stoll(a.dtstart); } catch (...) {}
        try { bi = std::stoll(b.dtstart); } catch (...) {}
        return ai > bi;
    });

    return events;
}

class CalendarPanel {
public:
    CalendarPanel()
        : m_title("Calendar")
        , m_capabilityId("list_calendar_events")
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
            m_events.clear();
        }

        auto capState = svc.GetState(m_capabilityId);
        if (m_phase == Phase::Loading && !capState.loading) {
            if (capState.success)
                m_events = ParseCalendarEvents(capState.data);
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
        } else if (m_phase == Phase::Loading && m_events.empty()) {
            ImGui::TextDisabled("Loading calendar events...");
        } else if (m_events.empty()) {
            ImGui::TextDisabled("No calendar events found on device.");
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

        if (ImGui::BeginTable("CalendarTable", 3,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders,
            ImVec2(0, 0)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 160);
            ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Calendar", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableHeadersRow();

            std::string filterLower = m_filter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            for (size_t i = 0; i < m_events.size(); i++) {
                const auto& ev = m_events[i];

                if (!m_filter.empty()) {
                    std::string haystack = ev.title + " " + ev.calendar_displayName + " " + ev.eventLocation;
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
                    m_detailEvent = m_events[i];
                    m_showDetailsPanel = true;
                }
                ImGui::SameLine();
                ImGui::Text("%s", ev.dtstartFormatted.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", ev.title.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", ev.calendar_displayName.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_events.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search events...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Calendar Event Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImGui::Text("%s", m_detailEvent.title.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(ID: %s)", m_detailEvent.id.c_str());

        ImGui::SeparatorText("Event Info");
        detailField("Title", m_detailEvent.title);
        detailField("Start", m_detailEvent.dtstartFormatted);
        detailField("End", m_detailEvent.dtendFormatted);

        if (!m_detailEvent.eventLocation.empty())
            detailField("Location", m_detailEvent.eventLocation);

        if (!m_detailEvent.description.empty())
            detailField("Description", m_detailEvent.description);

        if (!m_detailEvent.rawdata.empty()) {
            ImGui::SeparatorText("Raw Data");

            std::vector<std::string> allKeys;
            for (const auto& [key, _] : m_detailEvent.rawdata)
                allKeys.push_back(key);
            std::sort(allKeys.begin(), allKeys.end());

            if (ImGui::BeginTable("EventDataTable", 2,
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
                    auto it = m_detailEvent.rawdata.find(key);
                    if (it != m_detailEvent.rawdata.end())
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
    CalendarEvent m_detailEvent;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<CalendarEvent> m_events;

    std::string m_title;
    std::string m_capabilityId;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static CalendarPanel* s_panel = nullptr;

static void DrawCalendarPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterCalendarPanel() {
    s_panel = new CalendarPanel();
    PanelRegistry::Get().Register({
        "calendar",
        "Calendar",
        "Data",
        DrawCalendarPanel,
        s_panel->GetShowPtr(),
        {"calendar", "events", "schedule"}
    });
}
