#include "notifications.h"
#include "imgui.h"
#include "global_config.h"

void ShowNotifications() {
    ImGui::Text("Notifications Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
