#include "contacts.h"
#include "imgui.h"
#include "global_config.h"

void ShowContacts() {
    ImGui::Text("Contacts Screen");
    ImGui::Text("Selected Device ID: %s", GlobalConfig::GetSelectedDeviceId().c_str());
}
