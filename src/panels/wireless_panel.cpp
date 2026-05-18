#include "wireless_panel.h"
#include "core/registry/panel_registry.h"
#include "imgui.h"

static bool s_show = false;

static void DrawWirelessPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Wireless", &s_show)) {
        ImGui::Text("Coming soon...");
    }
    ImGui::End();
}

void RegisterWirelessPanel() {
    PanelRegistry::Get().Register({
        "wireless",
        "Wireless",
        "Devices",
        DrawWirelessPanel,
        &s_show,
        {}
    });
}
