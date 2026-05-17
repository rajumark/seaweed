#include "keyboard_list.h"
#include "imgui.h"
#include "global_config.h"

void ShowKeyboardList() {
    ImGui::Text("Keyboard List Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
