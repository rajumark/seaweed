#include "device_properties_panel.h"
#include "core/registry/panel_registry.h"
#include "core/capability/capability_service.h"
#include "global_config.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>

enum class PropertyType { System, Secure, Global };

struct DeviceProperty {
    PropertyType type;
    std::string typeLabel;
    std::string key;
    std::string value;
    std::string raw;
};

static std::string Trim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::vector<DeviceProperty> ParseProperties(const std::vector<std::string>& lines, PropertyType ptype) {
    std::vector<DeviceProperty> props;
    const char* typeLabel = "";
    switch (ptype) {
        case PropertyType::System: typeLabel = "System"; break;
        case PropertyType::Secure: typeLabel = "Secure"; break;
        case PropertyType::Global: typeLabel = "Global"; break;
    }

    for (const auto& line : lines) {
        std::string trimmed = Trim(line);
        if (trimmed.empty()) continue;

        auto eq = trimmed.find('=');
        if (eq == std::string::npos || eq == 0) continue;

        DeviceProperty p;
        p.type = ptype;
        p.typeLabel = typeLabel;
        p.raw = trimmed;
        p.key = Trim(trimmed.substr(0, eq));
        p.value = Trim(trimmed.substr(eq + 1));
        props.push_back(std::move(p));
    }
    return props;
}

class DevicePropertiesPanel {
public:
    DevicePropertiesPanel()
        : m_title("Device Properties")
        , m_capIdSys("list_properties_system")
        , m_capIdSec("list_properties_secure")
        , m_capIdGlb("list_properties_global")
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
            svc.InvalidateCache(m_capIdSys);
            svc.InvalidateCache(m_capIdSec);
            svc.InvalidateCache(m_capIdGlb);
            m_phase = Phase::Idle;
            m_properties.clear();
        }

        auto stateSys = svc.GetState(m_capIdSys);
        auto stateSec = svc.GetState(m_capIdSec);
        auto stateGlb = svc.GetState(m_capIdGlb);

        if (m_phase == Phase::LoadingAll && !stateSys.loading && !stateSec.loading && !stateGlb.loading) {
            m_properties.clear();
            if (stateSys.success) {
                auto parsed = ParseProperties(stateSys.data, PropertyType::System);
                m_properties.insert(m_properties.end(), parsed.begin(), parsed.end());
            }
            if (stateSec.success) {
                auto parsed = ParseProperties(stateSec.data, PropertyType::Secure);
                m_properties.insert(m_properties.end(), parsed.begin(), parsed.end());
            }
            if (stateGlb.success) {
                auto parsed = ParseProperties(stateGlb.data, PropertyType::Global);
                m_properties.insert(m_properties.end(), parsed.begin(), parsed.end());
            }
            m_phase = Phase::WithData;
        }

        if (m_phase == Phase::Idle && !m_cachedDeviceId.empty()) {
            svc.Execute(m_capIdSys, {m_cachedDeviceId});
            svc.Execute(m_capIdSec, {m_cachedDeviceId});
            svc.Execute(m_capIdGlb, {m_cachedDeviceId});
            m_phase = Phase::LoadingAll;
            m_lastRefresh = now;
        }

        if (m_phase != Phase::LoadingAll && !m_cachedDeviceId.empty()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_lastRefresh).count();
            if (elapsed >= m_refreshMs) {
                svc.Execute(m_capIdSys, {m_cachedDeviceId});
                svc.Execute(m_capIdSec, {m_cachedDeviceId});
                svc.Execute(m_capIdGlb, {m_cachedDeviceId});
                m_phase = Phase::LoadingAll;
                m_lastRefresh = now;
            }
        }

        if (m_cachedDeviceId.empty()) {
            ImGui::TextDisabled("No device selected. Connect a device first.");
        } else if (m_phase == Phase::LoadingAll && m_properties.empty()) {
            ImGui::TextDisabled("Loading device properties...");
        } else if (m_properties.empty()) {
            ImGui::TextDisabled("No properties found on device.");
        } else {
            RenderContent();
        }

        ImGui::End();

        RenderDetailWindow();
    }

    bool* GetShowPtr() { return &m_show; }

private:
    enum class Phase { Idle, LoadingAll, WithData };

    void RenderContent() {
        RenderToolbar();

        ImGui::Separator();

        if (ImGui::BeginTable("DevicePropertiesTable", 3,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders,
            ImVec2(0, 0)))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80);
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            std::string filterLower = m_filter;
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            for (size_t i = 0; i < m_properties.size(); i++) {
                const auto& prop = m_properties[i];

                if (!m_filter.empty()) {
                    std::string haystack = prop.key + " " + prop.value + " " + prop.typeLabel;
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
                    m_detailProp = m_properties[i];
                    m_showDetailsPanel = true;
                }
                ImGui::SameLine();

                ImVec4 typeColor;
                switch (prop.type) {
                    case PropertyType::System: typeColor = ImVec4(0.2f, 0.8f, 0.8f, 1.0f); break;
                    case PropertyType::Secure: typeColor = ImVec4(0.8f, 0.6f, 0.2f, 1.0f); break;
                    case PropertyType::Global: typeColor = ImVec4(0.2f, 0.6f, 0.8f, 1.0f); break;
                }
                ImGui::TextColored(typeColor, "%s", prop.typeLabel.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", prop.key.c_str());

                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", prop.value.c_str());

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_properties.size());
        ImGui::SameLine();
        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search properties...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel) return;
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Device Property Details", &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        ImVec4 typeColor;
        switch (m_detailProp.type) {
            case PropertyType::System: typeColor = ImVec4(0.2f, 0.8f, 0.8f, 1.0f); break;
            case PropertyType::Secure: typeColor = ImVec4(0.8f, 0.6f, 0.2f, 1.0f); break;
            case PropertyType::Global: typeColor = ImVec4(0.2f, 0.6f, 0.8f, 1.0f); break;
        }

        ImGui::TextColored(typeColor, "%s", m_detailProp.typeLabel.c_str());

        ImGui::SeparatorText("Property");
        detailField("Key", m_detailProp.key);
        detailField("Value", m_detailProp.value);
        detailField("Raw", m_detailProp.raw);

        ImGui::End();
    }

    void detailField(const char* label, const std::string& value) {
        ImGui::Text("%s: ", label);
        ImGui::SameLine();
        ImGui::TextWrapped("%s", value.c_str());
    }

    void TriggerRefresh() {
        auto& svc = CapabilityService::Get();
        svc.InvalidateCache(m_capIdSys);
        svc.InvalidateCache(m_capIdSec);
        svc.InvalidateCache(m_capIdGlb);
        m_phase = Phase::Idle;
    }

    bool m_show = false;
    bool m_showDetailsPanel = false;
    DeviceProperty m_detailProp;
    int m_selectedIndex = -1;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<DeviceProperty> m_properties;

    std::string m_title;
    std::string m_capIdSys;
    std::string m_capIdSec;
    std::string m_capIdGlb;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
};

static DevicePropertiesPanel* s_panel = nullptr;

static void DrawDevicePropertiesPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterDevicePropertiesPanel() {
    s_panel = new DevicePropertiesPanel();
    PanelRegistry::Get().Register({
        "device_properties",
        "Device Properties",
        "Data",
        DrawDevicePropertiesPanel,
        s_panel->GetShowPtr(),
        {"device properties", "settings", "system", "secure", "global"}
    });
}
