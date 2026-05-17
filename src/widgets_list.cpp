#include "widgets_list.h"
#include "imgui.h"
#include "global_config.h"

void ShowWidgetsList() {
    ImGui::Text("Widgets List Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
