#include "device_list_panel.h"
#include "core/registry/panel_registry.h"
#include "global_config.h"
#include "device_manager.h"
#include "imgui.h"
#include <string>

static bool s_show = false;

static void DrawDeviceListPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Device List", &s_show)) {
        auto devices = GlobalConfig::GetDevices();
        std::string selectedId = GlobalConfig::GetSelectedDeviceId();

        if (devices.empty()) {
            ImGui::TextDisabled("No devices found. Connect a device or start an emulator.");
        } else {
            ImGui::Text("Devices: %zu", devices.size());
            ImGui::Separator();

            if (ImGui::BeginTable("devices", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Device");
                ImGui::TableSetupColumn("ID");
                ImGui::TableSetupColumn("OS");
                ImGui::TableSetupColumn("State");
                ImGui::TableHeadersRow();

                for (int i = 0; i < (int)devices.size(); i++) {
                    const auto& d = devices[i];
                    bool isSelected = (d.deviceId == selectedId);

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    ImGui::PushID(i);
                    std::string label = (isSelected ? "[x] " : "[ ] ") + d.deviceName;
                    if (ImGui::Selectable(label.c_str(), isSelected,
                        ImGuiSelectableFlags_SpanAllColumns))
                    {
                        GlobalConfig::SetSelectedDeviceId(d.deviceId);
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", d.deviceId.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", d.osVersion.empty() ? "-" : d.osVersion.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", AdbStateToString(d.state));
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

void RegisterDeviceListPanel() {
    PanelRegistry::Get().Register({
        "device_list",
        "Device List",
        "Devices",
        DrawDeviceListPanel,
        &s_show,
        {}
    });
}
