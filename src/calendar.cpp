#include "calendar.h"
#include "imgui.h"
#include "global_config.h"

void ShowCalendar() {
    ImGui::Text("Calendar Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
