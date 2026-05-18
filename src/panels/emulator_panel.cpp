#include "emulator_panel.h"
#include "core/registry/panel_registry.h"
#include "emulator_manager.h"
#include "imgui.h"
#include <string>

static bool s_show = false;

static void DrawEmulatorPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Emulator", &s_show)) {
        auto& em = EmulatorManager::GetInstance();
        bool scanning = em.IsScanning();

        if (ImGui::Button(scanning ? "Scanning..." : "Refresh", ImVec2(120, 0))) {
            if (!scanning) em.ScanEmulators();
        }
        ImGui::SameLine();
        std::string emPath = em.GetEmulatorPath();
        if (!emPath.empty()) {
            ImGui::TextDisabled("Emulator: %s", emPath.c_str());
        } else {
            ImGui::TextDisabled("Emulator not found in default paths");
        }

        ImGui::Separator();

        auto emulators = em.GetEmulators();
        if (emulators.empty() && !scanning) {
            ImGui::TextDisabled("No emulators found. Press Refresh to scan.");
        } else if (scanning) {
            ImGui::TextDisabled("Scanning for emulators...");
        } else {
            if (ImGui::BeginTable("emulators", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Device ID");
                ImGui::TableSetupColumn("Actions");
                ImGui::TableHeadersRow();

                for (int i = 0; i < (int)emulators.size(); i++) {
                    const auto& e = emulators[i];

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", e.name.c_str());
                    ImGui::TableNextColumn();
                    if (e.isRunning) {
                        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Running");
                    } else {
                        ImGui::Text("Stopped");
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", e.isRunning ? e.deviceId.c_str() : "-");
                    ImGui::TableNextColumn();

                    ImGui::PushID(i);
                    if (e.isRunning) {
                        if (ImGui::Button("Stop", ImVec2(60, 0))) {
                            em.StopEmulator(e.deviceId);
                        }
                    } else {
                        if (ImGui::Button("Start", ImVec2(60, 0))) {
                            em.StartEmulator(e.name);
                        }
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

void RegisterEmulatorPanel() {
    PanelRegistry::Get().Register({
        "emulator",
        "Emulator",
        "Devices",
        DrawEmulatorPanel,
        &s_show,
        {}
    });
}
