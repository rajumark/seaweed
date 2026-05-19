#include "messages_panel.h"
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

struct Message {
    std::string id;
    std::string type;
    std::string typeLabel;
    std::string address;
    std::string body;
    std::string date;
    std::string dateFormatted;
    std::string read;
    std::string readLabel;
    std::string dateSent;
    std::map<std::string, std::string> rawdata;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::string MsgTypeLabel(const std::string& val) {
    int t = 0;
    try { t = std::stoi(val); } catch (...) {}
    switch (t) {
        case 1: return "Incoming";
        case 2: return "Sent";
        case 3: return "Draft";
        case 4: return "Failed";
        default: return "Unknown";
    }
}

static ImVec4 MsgTypeColor(const std::string& val) {
    int t = 0;
    try { t = std::stoi(val); } catch (...) {}
    switch (t) {
        case 1: return ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        case 2: return ImVec4(0.2f, 0.5f, 1.0f, 1.0f);
        case 3: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        case 4: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

static std::string ReadLabel(const std::string& val) {
    int r = 0;
    try { r = std::stoi(val); } catch (...) {}
    return r == 1 ? "Read" : "Unread";
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

static std::map<std::string, std::string> ParseMsgRow(const std::string& line) {
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

static std::vector<Message> ParseMessages(const std::vector<std::string>& lines, bool showOriginal) {
    std::vector<Message> msgs;

    for (const auto& line : lines) {
        auto raw = ParseMsgRow(line);
        if (raw.empty()) continue;

        Message m;
        m.rawdata = raw;
        m.id = raw["_id"];
        m.type = raw["type"];
        m.address = raw["address"];
        m.body = raw["body"];
        m.read = raw["read"];
        m.dateSent = raw["date_sent"];

        if (showOriginal) {
            m.typeLabel = m.type;
            m.readLabel = m.read;
            std::string rel;
            m.dateFormatted = raw["date"];
        } else {
            m.typeLabel = MsgTypeLabel(raw["type"]);
            m.readLabel = ReadLabel(raw["read"]);
            std::string rel;
            m.dateFormatted = FormatTimestamp(raw["date"], rel);
            m.date = rel;
        }

        msgs.push_back(std::move(m));
    }

    std::sort(msgs.begin(), msgs.end(), [](const Message& a, const Message& b) {
        int ai = 0, bi = 0;
        try { ai = std::stoi(a.id); } catch (...) {}
        try { bi = std::stoi(b.id); } catch (...) {}
        return ai > bi;
    });

    return msgs;
}

class MessagesPanel {
public:
    MessagesPanel()
        : m_title("Messages")
        , m_capabilityId("list_messages")
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
            m_messages.clear();
        }

        auto capState = svc.GetState(m_capabilityId);
        if (m_phase == Phase::Loading && !capState.loading) {
            if (capState.success)
                m_messages = ParseMessages(capState.data, m_showOriginal);
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
        } else if (m_phase == Phase::Loading && m_messages.empty()) {
            ImGui::TextDisabled("Loading messages...");
        } else if (m_messages.empty()) {
            ImGui::TextDisabled("No messages found on device.");
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

        if (ImGui::BeginTable("MsgTable", 3,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders,
            ImVec2(0, 0)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 160);
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 150);
            ImGui::TableSetupColumn("Body", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            std::string filterLower = m_filter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            for (size_t i = 0; i < m_messages.size(); i++) {
                const auto& msg = m_messages[i];

                if (!m_filter.empty()) {
                    std::string haystack = msg.address + " " + msg.body + " " + msg.typeLabel;
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

                ImVec4 col = MsgTypeColor(msg.type);

                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextColumn();
                ImGui::Selectable("##sel", &isSelected,
                    ImGuiSelectableFlags_SpanAllColumns);
                if (ImGui::IsItemClicked() ||
                    (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                    m_selectedIndex = static_cast<int>(i);
                    m_detailMsg = m_messages[i];
                    m_showDetailsPanel = true;
                }
                ImGui::SameLine();
                ImGui::Text("%s", msg.dateFormatted.c_str());

                ImGui::TableNextColumn();
                ImGui::TextColored(col, "%s", msg.address.c_str());

                ImGui::TableNextColumn();
                std::string bodyPreview = msg.body.length() > 80
                    ? msg.body.substr(0, 80) + "..."
                    : msg.body;
                ImGui::TextWrapped("%s", bodyPreview.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_messages.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search messages...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }

        ImGui::SameLine();
        ImGui::Checkbox("Show Original", &m_showOriginal);
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            m_messages = ParseMessages(
                CapabilityService::Get().GetState(m_capabilityId).data,
                m_showOriginal);
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Message Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImGui::TextColored(MsgTypeColor(m_detailMsg.type), "%s", m_detailMsg.typeLabel.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(ID: %s)", m_detailMsg.id.c_str());

        ImGui::SeparatorText("Message Info");
        detailField("Address", m_detailMsg.address);
        if (!m_detailMsg.dateFormatted.empty()) {
            detailField("Date", m_detailMsg.dateFormatted);
            if (!m_detailMsg.date.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s)", m_detailMsg.date.c_str());
            }
        }
        detailField("Read", m_detailMsg.readLabel);

        if (!m_detailMsg.body.empty()) {
            ImGui::SeparatorText("Body");
            ImGui::TextWrapped("%s", m_detailMsg.body.c_str());
        }

        if (!m_detailMsg.rawdata.empty()) {
            ImGui::SeparatorText("Raw Data");

            std::vector<std::string> allKeys;
            for (const auto& [key, _] : m_detailMsg.rawdata)
                allKeys.push_back(key);
            std::sort(allKeys.begin(), allKeys.end());

            if (ImGui::BeginTable("MsgDataTable", 2,
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
                    auto it = m_detailMsg.rawdata.find(key);
                    if (it != m_detailMsg.rawdata.end())
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
    bool m_showOriginal = false;
    Message m_detailMsg;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<Message> m_messages;

    std::string m_title;
    std::string m_capabilityId;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static MessagesPanel* s_panel = nullptr;

static void DrawMessagesPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterMessagesPanel() {
    s_panel = new MessagesPanel();
    PanelRegistry::Get().Register({
        "messages",
        "Messages",
        "Data",
        DrawMessagesPanel,
        s_panel->GetShowPtr(),
        {"messages", "sms", "text", "conversations"}
    });
}
