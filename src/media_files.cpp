#include "media_files.h"
#include "imgui.h"
#include "global_config.h"

void ShowMediaFiles() {
    ImGui::Text("Media Files Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
