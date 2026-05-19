#include "media_panel.h"
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

struct MediaItem {
    std::string id;
    std::string displayName;
    std::string mimeType;
    std::string size;
    std::string sizeFormatted;
    std::string dateAdded;
    std::string dateAddedFormatted;
    std::string resolution;
    std::string duration;
    std::map<std::string, std::string> rawdata;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::string FormatFileSize(const std::string& val) {
    long long bytes = 0;
    try { bytes = std::stoll(val); } catch (...) { return val; }
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024*1024) return std::to_string(bytes/1024) + " KB";
    return std::to_string(bytes/(1024*1024)) + " MB";
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

static std::map<std::string, std::string> ParseMediaRow(const std::string& line) {
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

static std::vector<MediaItem> ParseMediaItems(const std::vector<std::string>& lines) {
    std::vector<MediaItem> items;

    for (const auto& line : lines) {
        auto raw = ParseMediaRow(line);
        if (raw.empty()) continue;

        MediaItem m;
        m.rawdata = raw;
        m.id = raw["_id"];
        m.displayName = raw["_display_name"];
        m.mimeType = raw["mime_type"];
        m.size = raw["_size"];
        m.sizeFormatted = FormatFileSize(raw["_size"]);
        m.resolution = raw["resolution"];
        m.duration = raw["duration"];

        // date_added is epoch seconds, convert to ms for FormatTimestamp
        std::string dateMs = raw["date_added"];
        if (!dateMs.empty()) {
            long long secs = 0;
            try { secs = std::stoll(dateMs); } catch (...) {}
            dateMs = std::to_string(secs * 1000);
        }

        std::string relative;
        m.dateAddedFormatted = FormatTimestamp(dateMs, relative);
        m.dateAdded = relative;

        items.push_back(std::move(m));
    }

    std::sort(items.begin(), items.end(), [](const MediaItem& a, const MediaItem& b) {
        int ai = 0, bi = 0;
        try { ai = std::stoi(a.id); } catch (...) {}
        try { bi = std::stoi(b.id); } catch (...) {}
        return ai > bi;
    });

    return items;
}

class MediaPanel {
public:
    MediaPanel()
        : m_title("Media")
        , m_capabilityId("list_media")
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
            m_items.clear();
        }

        auto capState = svc.GetState(m_capabilityId);
        if (m_phase == Phase::Loading && !capState.loading) {
            if (capState.success)
                m_items = ParseMediaItems(capState.data);
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
        } else if (m_phase == Phase::Loading && m_items.empty()) {
            ImGui::TextDisabled("Loading media...");
        } else if (m_items.empty()) {
            ImGui::TextDisabled("No media found on device.");
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

        if (ImGui::BeginTable("MediaTable", 4,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders,
            ImVec2(0, 0)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 160);
            ImGui::TableHeadersRow();

            std::string filterLower = m_filter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            for (size_t i = 0; i < m_items.size(); i++) {
                const auto& item = m_items[i];

                if (!m_filter.empty()) {
                    std::string haystack = item.displayName + " " + item.mimeType;
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

                std::string displayName = item.displayName.empty() ? "-" : item.displayName;

                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextColumn();
                ImGui::Selectable("##sel", &isSelected,
                    ImGuiSelectableFlags_SpanAllColumns);
                if (ImGui::IsItemClicked() ||
                    (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                    m_selectedIndex = static_cast<int>(i);
                    m_detailItem = m_items[i];
                    m_showDetailsPanel = true;
                }
                ImGui::SameLine();
                ImGui::Text("%s", displayName.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", item.mimeType.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", item.sizeFormatted.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", item.dateAddedFormatted.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_items.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search media...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Media Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImGui::Text("%s", m_detailItem.displayName.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(ID: %s)", m_detailItem.id.c_str());

        ImGui::SeparatorText("Media Info");
        detailField("Name", m_detailItem.displayName.empty() ? "-" : m_detailItem.displayName);
        detailField("MIME Type", m_detailItem.mimeType);
        detailField("Size", m_detailItem.sizeFormatted);
        ImGui::SameLine();
        ImGui::TextDisabled("(%s bytes)", m_detailItem.size.c_str());
        detailField("Date Added", m_detailItem.dateAddedFormatted);
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", m_detailItem.dateAdded.c_str());

        if (!m_detailItem.resolution.empty())
            detailField("Resolution", m_detailItem.resolution);
        if (!m_detailItem.duration.empty())
            detailField("Duration", m_detailItem.duration);

        auto pathIt = m_detailItem.rawdata.find("_data");
        if (pathIt != m_detailItem.rawdata.end())
            detailField("Path", pathIt->second);

        if (!m_detailItem.rawdata.empty()) {
            ImGui::SeparatorText("Raw Data");

            std::vector<std::string> allKeys;
            for (const auto& [key, _] : m_detailItem.rawdata)
                allKeys.push_back(key);
            std::sort(allKeys.begin(), allKeys.end());

            if (ImGui::BeginTable("MediaDataTable", 2,
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
                    auto it = m_detailItem.rawdata.find(key);
                    if (it != m_detailItem.rawdata.end())
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
    MediaItem m_detailItem;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<MediaItem> m_items;

    std::string m_title;
    std::string m_capabilityId;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static MediaPanel* s_panel = nullptr;

static void DrawMediaPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterMediaPanel() {
    s_panel = new MediaPanel();
    PanelRegistry::Get().Register({
        "media",
        "Media",
        "Data",
        DrawMediaPanel,
        s_panel->GetShowPtr(),
        {"media", "images", "photos"}
    });
}
