#include "apps_panel.h"
#include "core/registry/panel_registry.h"
#include "core/ui/async_data_panel.h"
#include "apps_action_cook_helper.h"
#include "imgui.h"
#include <string>
#include <vector>

class AppsPanel : public AsyncDataPanel {
public:
    AppsPanel()
        : AsyncDataPanel("Apps", "list_packages", 4000)
    {
    }

protected:
    void RenderContent() override {
        ImGui::Text("Total: %zu", Displayed().size());
        ImGui::Separator();
        ImGui::BeginChild("PackagesList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& pkg : Displayed()) {
            bool selected = (m_selectedPkg == pkg);
            ImGui::Selectable(pkg.c_str(), &selected, ImGuiSelectableFlags_None);
            if (ImGui::BeginPopupContextItem())
                HandleContextMenu(pkg);
        }
        ImGui::EndChild();
    }

private:
    void HandleContextMenu(const std::string& pkg) {
        if (ImGui::MenuItem("Open"))
            AppsActionCookHelper::StartAppAsync(pkg);
        if (ImGui::MenuItem("Force Stop"))
            AppsActionCookHelper::ForceStopAppAsync(pkg);
        if (ImGui::MenuItem("Restart"))
            AppsActionCookHelper::RestartAppAsync(pkg);
        ImGui::Separator();
        if (ImGui::MenuItem("Uninstall"))
            AppsActionCookHelper::UninstallAppAsync(pkg);
        if (ImGui::MenuItem("Clear Data"))
            AppsActionCookHelper::ClearAppDataAsync(pkg);
        ImGui::Separator();
        if (ImGui::MenuItem("Enable"))
            AppsActionCookHelper::EnableAppAsync(pkg);
        if (ImGui::MenuItem("Disable"))
            AppsActionCookHelper::DisableAppAsync(pkg);
        ImGui::Separator();
        if (ImGui::MenuItem("Open App Info"))
            AppsActionCookHelper::OpenAppInfoAsync(pkg);
        if (ImGui::MenuItem("Copy Package Name"))
            AppsActionCookHelper::CopyPackageName(pkg);
        ImGui::EndPopup();
    }

    std::string m_selectedPkg;
};

static AppsPanel* s_panel = nullptr;

static void DrawAppsPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterAppsPanel() {
    s_panel = new AppsPanel();
    PanelRegistry::Get().Register({
        "apps",
        "Apps",
        "Data",
        DrawAppsPanel,
        s_panel->GetShowPtr(),
        {"packages", "applications", "installed"}
    });
}
