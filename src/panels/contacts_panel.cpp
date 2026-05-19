#include "contacts_panel.h"
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

struct Contact {
    std::string contact_id;
    std::string displayName;
    std::vector<std::string> numbers;
    std::vector<std::map<std::string, std::string>> rawdata;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::map<std::string, std::string> ParseContactRow(const std::string& line) {
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

static std::vector<Contact> ParseContacts(const std::vector<std::string>& lines) {
    std::map<std::string, std::vector<std::map<std::string, std::string>>> grouped;

    for (const auto& line : lines) {
        auto row = ParseContactRow(line);
        auto it = row.find("contact_id");
        if (it != row.end() && !it->second.empty())
            grouped[it->second].push_back(std::move(row));
    }

    std::vector<Contact> contacts;
    for (auto& [cid, rows] : grouped) {
        Contact c;
        c.contact_id = cid;

        for (auto& r : rows) {
            auto dn = r.find("display_name");
            if (dn != r.end() && !dn->second.empty()) {
                c.displayName = dn->second;
                break;
            }
        }

        for (auto& r : rows) {
            auto mt = r.find("mimetype");
            if (mt != r.end() && mt->second == "vnd.android.cursor.item/phone_v2") {
                auto d1 = r.find("data1");
                if (d1 != r.end() && !d1->second.empty())
                    c.numbers.push_back(d1->second);
            }
        }

        c.rawdata = std::move(rows);
        contacts.push_back(std::move(c));
    }

    std::sort(contacts.begin(), contacts.end(), [](const Contact& a, const Contact& b) {
        int ai = 0, bi = 0;
        try { ai = std::stoi(a.contact_id); } catch (...) {}
        try { bi = std::stoi(b.contact_id); } catch (...) {}
        return ai > bi;
    });

    return contacts;
}

class ContactsPanel {
public:
    ContactsPanel()
        : m_title("Contacts")
        , m_capabilityId("list_contacts")
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
            m_contacts.clear();
        }

        auto capState = svc.GetState(m_capabilityId);
        if (m_phase == Phase::Loading && !capState.loading) {
            if (capState.success)
                m_contacts = ParseContacts(capState.data);
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
        } else if (m_phase == Phase::Loading && m_contacts.empty()) {
            ImGui::TextDisabled("Loading contacts...");
        } else if (m_contacts.empty()) {
            ImGui::TextDisabled("No contacts found on device.");
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

        ImGui::BeginChild("ContactList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        std::string filterLower = m_filter;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        for (size_t i = 0; i < m_contacts.size(); i++) {
            const auto& c = m_contacts[i];
            std::string label = c.displayName.empty()
                ? (c.numbers.empty() ? "Unknown" : c.numbers[0])
                : c.displayName;

            if (!m_filter.empty()) {
                std::string labelLower = label;
                std::transform(labelLower.begin(), labelLower.end(), labelLower.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                bool match = labelLower.find(filterLower) != std::string::npos;
                if (!match) {
                    for (const auto& num : c.numbers) {
                        std::string numLower = num;
                        std::transform(numLower.begin(), numLower.end(), numLower.begin(),
                                       [](unsigned char c) { return std::tolower(c); });
                        if (numLower.find(filterLower) != std::string::npos) {
                            match = true;
                            break;
                        }
                    }
                }
                if (!match) continue;
            }

            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(label.c_str(), m_selectedIndex == static_cast<int>(i))) {
                m_selectedIndex = static_cast<int>(i);
                m_detailContact = m_contacts[i];
                m_showDetailsPanel = true;
            }
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_contacts.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search contacts...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Contact Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImGui::Text("%s", m_detailContact.displayName.empty() ? "Unknown" : m_detailContact.displayName.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(ID: %s)", m_detailContact.contact_id.c_str());

        if (!m_detailContact.numbers.empty()) {
            ImGui::SeparatorText("Phone Numbers");
            for (const auto& num : m_detailContact.numbers)
                ImGui::Text("%s", num.c_str());
        }

        if (!m_detailContact.rawdata.empty()) {
            ImGui::SeparatorText("Contact Data");

            std::vector<std::string> allKeys;
            for (const auto& row : m_detailContact.rawdata)
                for (const auto& [key, _] : row)
                    if (std::find(allKeys.begin(), allKeys.end(), key) == allKeys.end())
                        allKeys.push_back(key);

            std::sort(allKeys.begin(), allKeys.end());

            if (ImGui::BeginTable("ContactDataTable", static_cast<int>(allKeys.size()) + 1,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX |
                ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupScrollFreeze(1, 1);
                ImGui::TableSetupColumn("#");
                for (const auto& key : allKeys)
                    ImGui::TableSetupColumn(key.c_str());
                ImGui::TableHeadersRow();

                for (size_t ri = 0; ri < m_detailContact.rawdata.size(); ri++) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%zu", ri);
                    for (const auto& key : allKeys) {
                        ImGui::TableNextColumn();
                        auto it = m_detailContact.rawdata[ri].find(key);
                        if (it != m_detailContact.rawdata[ri].end())
                            ImGui::TextWrapped("%s", it->second.c_str());
                    }
                }
                ImGui::EndTable();
            }
        }

        ImGui::End();
    }

    void TriggerRefresh() {
        CapabilityService::Get().InvalidateCache(m_capabilityId);
        m_phase = Phase::Idle;
    }

    bool m_show = false;
    bool m_showDetailsPanel = false;
    Contact m_detailContact;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<Contact> m_contacts;

    std::string m_title;
    std::string m_capabilityId;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static ContactsPanel* s_panel = nullptr;

static void DrawContactsPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterContactsPanel() {
    s_panel = new ContactsPanel();
    PanelRegistry::Get().Register({
        "contacts",
        "Contacts",
        "Data",
        DrawContactsPanel,
        s_panel->GetShowPtr(),
        {"contacts", "people", "address book", "phone"}
    });
}
