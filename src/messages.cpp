#include "messages.h"
#include "imgui.h"
#include "global_config.h"

void ShowMessages() {
    ImGui::Text("Messages Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
