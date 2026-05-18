#include "apps_panel.h"
#include "core/registry/panel_registry.h"
#include "apps_cook_helper.h"
#include "imgui.h"
#include <string>
#include <chrono>
#include <vector>

static bool s_show = false;
static std::vector<std::string> s_displayed;
static std::chrono::steady_clock::time_point s_lastRefresh;
static bool s_wasLoading = false;

static void DrawAppsPanel() {
    if (!s_show) return;

    if (ImGui::Begin("Apps", &s_show)) {
        bool isLoading = AppsCookHelper::IsLoading();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastRefresh).count();

        if (elapsed >= 4000 && !isLoading) {
            AppsCookHelper::LoadPackagesListAsync();
            s_lastRefresh = now;
            isLoading = true;
        }

        if (s_wasLoading && !isLoading) {
            std::vector<std::string> fresh = AppsCookHelper::GetPackages();
            if (fresh != s_displayed)
                s_displayed = std::move(fresh);
        }
        s_wasLoading = isLoading;

        ImGui::Separator();

        if (s_displayed.empty() && !isLoading) {
            const std::string& err = AppsCookHelper::GetLastError();
            if (!err.empty())
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", err.c_str());
            else
                ImGui::TextDisabled("No packages found. Select a device and press Refresh.");
        } else {
            ImGui::Text("Total: %zu", s_displayed.size());
            ImGui::Separator();
            ImGui::BeginChild("PackagesList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto& pkg : s_displayed)
                ImGui::Text("%s", pkg.c_str());
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
