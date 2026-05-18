#include "apps_panel.h"
#include "core/registry/panel_registry.h"
#include "apps_cook_helper.h"
#include "imgui.h"
#include <string>

static bool s_show = false;
static bool s_autoLoaded = false;

static void DrawAppsPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Apps", &s_show)) {
        if (!s_autoLoaded) {
            AppsCookHelper::LoadPackagesListAsync();
            s_autoLoaded = true;
        }

        if (ImGui::Button("Refresh")) {
            AppsCookHelper::LoadPackagesListAsync();
        }

        ImGui::SameLine();
        if (AppsCookHelper::IsLoading()) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading...");
        }

        const std::string& lastError = AppsCookHelper::GetLastError();
        if (!lastError.empty()) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", lastError.c_str());
            ImGui::Separator();
        }

        ImGui::Separator();

        const auto& packages = AppsCookHelper::GetPackages();
        if (packages.empty() && !AppsCookHelper::IsLoading()) {
            ImGui::TextDisabled("No packages found. Select a device and press Refresh.");
        } else {
            ImGui::Text("Total: %zu", packages.size());
            ImGui::Separator();

            ImGui::BeginChild("PackagesList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& pkg : packages) {
                ImGui::Text("%s", pkg.c_str());
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
}

void RegisterAppsPanel() {
    PanelRegistry::Get().Register({
        "apps",
        "Apps",
        "Data",
        DrawAppsPanel,
        &s_show,
        {"packages", "applications", "installed"}
    });
}
