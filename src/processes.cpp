#include "processes.h"
#include "imgui.h"
#include "global_config.h"

void ShowProcesses() {
    ImGui::Text("Processes Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
