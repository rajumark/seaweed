#include "install_app.h"
#include "imgui.h"
#include "global_config.h"

void ShowInstallApp() {
    ImGui::Text("Install App Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
