#include "theme_panel.h"
#include "core/registry/panel_registry.h"
#include "global_config.h"
#include "imgui.h"

static bool s_show = false;

static void DrawThemePanel() {
    if (!s_show) return;

    if (ImGui::Begin("Theme", &s_show)) {
        Theme currentTheme = GlobalConfig::GetCurrentTheme();

        if (ImGui::RadioButton("Dark Theme", currentTheme == Theme::DARK))
            GlobalConfig::SetTheme(Theme::DARK);
        if (ImGui::RadioButton("Light Theme", currentTheme == Theme::LIGHT))
            GlobalConfig::SetTheme(Theme::LIGHT);
    }
    ImGui::End();
}

void RegisterThemePanel() {
    PanelRegistry::Get().Register({
        "theme",
        "Theme",
        "Seaweed",
        DrawThemePanel,
        &s_show,
        {"dark", "light", "colors", "appearance", "style"}
    });
}
