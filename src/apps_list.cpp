#include "imgui.h"
#include "apps_cook_helper.h"
#include "apps_action_cook_helper.h"
#include "apps_pinned_store.h"
#include "confirmation_dialog.h"
#include <chrono>
#include <string>
#include <unordered_set>

static char g_searchBuffer[256] = ""; // Search input buffer
static auto g_lastRefreshTime = std::chrono::steady_clock::now();
static bool g_isFirstLoad = true;
static std::string g_selectedPackage = ""; // Track which package is selected for context menu

void ShowAppsList() {
    ImGui::TextUnformatted("Installed Packages");
    ImGui::Separator();

    // Auto-refresh every 2 seconds when screen is visible
    auto currentTime = std::chrono::steady_clock::now();
    auto timeSinceLastRefresh = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - g_lastRefreshTime);
    
    // Initial load or refresh every 2 seconds
    if (g_isFirstLoad || timeSinceLastRefresh.count() >= 2000) {
        // Check if previous call is in progress
        if (!AppsCookHelper::IsLoading()) {
            AppsCookHelper::LoadPackagesListAsync();
            g_lastRefreshTime = currentTime;
        }
        g_isFirstLoad = false;
    }
    
    // Search input field
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputTextWithHint("##SearchPackages", "Search", g_searchBuffer, sizeof(g_searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Optional: Add any immediate search behavior here if needed
    }

    ImGui::SameLine();
    
    // Filter combo box
    ImGui::SetNextItemWidth(130);
    const char* filterItems[] = { "All Apps", "System Apps", "User Apps" };
    int currentFilter = AppsCookHelper::GetFilter();
    if (ImGui::Combo("##FilterApps", &currentFilter, filterItems, IM_ARRAYSIZE(filterItems))) {
        // Reload packages when filter changes
        AppsCookHelper::SetFilter(currentFilter);
        if (!AppsCookHelper::IsLoading()) {
            AppsCookHelper::LoadPackagesListAsync();
        }
    }

    ImGui::Spacing();

    // Show error if present
    const std::string& lastError = AppsCookHelper::GetLastError();
    if (!lastError.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", lastError.c_str());
        return;
    }

    // Get filtered packages based on search term
    std::vector<std::string> filteredPackages = AppsCookHelper::GetFilteredPackages(std::string(g_searchBuffer));
    const std::vector<std::string>& allPackages = AppsCookHelper::GetPackages();

    // Get pinned count for display
    size_t pinnedCount = AppsPinnedStore::GetPinnedCount();
    ImGui::Text("Total: %d (Filtered: %d, Pinned: %d)", (int)allPackages.size(), (int)filteredPackages.size(), (int)pinnedCount);
    
    // Show status messages
    StatusInfo currentStatus = AppsActionCookHelper::GetCurrentStatus();
    if (currentStatus.IsVisible()) {
        ImVec4 statusColor;
        
        switch (currentStatus.status) {
            case ActionStatus::Running:
                statusColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
                break;
            case ActionStatus::Success:
                statusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
                break;
            case ActionStatus::Failed:
                statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
                break;
            default:
                statusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
                break;
        }
        
        ImGui::SameLine();
        ImGui::TextColored(statusColor, " | %s", currentStatus.message.c_str());
        
        // Show tooltip with full details on hover
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
            ImGui::Text("%s", currentStatus.actionName.c_str());
            ImGui::PopStyleColor();
            
            if (!currentStatus.packageName.empty()) {
                ImGui::Text("Package: %s", currentStatus.packageName.c_str());
            }
            ImGui::Separator();
            ImGui::TextWrapped("%s", currentStatus.fullDetails.c_str());
            
            // Show copy button for failed actions
            if (currentStatus.status == ActionStatus::Failed) {
                ImGui::Separator();
                if (ImGui::Button("Copy Error Details")) {
                    AppsActionCookHelper::CopyStatusToClipboard();
                }
            }
            ImGui::EndTooltip();
        }
    }

    ImGui::BeginChild("PackagesList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Get pinned packages once and cache them
    std::vector<std::string> pinnedPackages = AppsPinnedStore::GetPinnedPackages();
    
    // Create a fast lookup set for pinned packages
    std::unordered_set<std::string> pinnedSet(pinnedPackages.begin(), pinnedPackages.end());
    
    // Filter pinned packages based on search (only if there are pinned packages)
    std::vector<std::string> filteredPinnedPackages;
    if (!pinnedPackages.empty()) {
        filteredPinnedPackages.reserve(pinnedPackages.size()); // Pre-allocate for better performance
        for (const auto& pkg : pinnedPackages) {
            if (std::string(g_searchBuffer).empty() || 
                pkg.find(g_searchBuffer) != std::string::npos) {
                filteredPinnedPackages.push_back(pkg);
            }
        }
    }
    
    // Show pinned apps section if there are any pinned apps 
    if (!filteredPinnedPackages.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.8f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.9f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));
        
        if (ImGui::CollapsingHeader("Pinned Apps", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& pkg : filteredPinnedPackages) {
                bool isSelected = (g_selectedPackage == pkg);
                
                // Use Selectable for default ImGui hover highlighting
                ImGui::Selectable(pkg.c_str(), isSelected, ImGuiSelectableFlags_None);
                
                // Right-click context menu
                if (ImGui::BeginPopupContextItem(("##ContextMenu_" + pkg).c_str())) {
                    g_selectedPackage = pkg;
                    
                    if (ImGui::MenuItem("Open")) {
                        AppsActionCookHelper::StartAppAsync(pkg);
                    }
                    if (ImGui::MenuItem("Force Stop")) {
                        AppsActionCookHelper::ForceStopAppAsync(pkg);
                    }
                    if (ImGui::MenuItem("Restart")) {
                        AppsActionCookHelper::RestartAppAsync(pkg);
                    }
                    if (ImGui::MenuItem("Uninstall")) {
                        if (ShouldShowConfirmation(ConfirmationType::Uninstall)) {
                            OpenConfirmationDialog(ConfirmationType::Uninstall, pkg, 
                                [pkg]() { AppsActionCookHelper::UninstallAppAsync(pkg); });
                        } else {
                            AppsActionCookHelper::UninstallAppAsync(pkg);
                        }
                    }
                    if (ImGui::MenuItem("Clear Data")) {
                        if (ShouldShowConfirmation(ConfirmationType::ClearData)) {
                            OpenConfirmationDialog(ConfirmationType::ClearData, pkg, 
                                [pkg]() { AppsActionCookHelper::ClearAppDataAsync(pkg); });
                        } else {
                            AppsActionCookHelper::ClearAppDataAsync(pkg);
                        }
                    }
                    if (ImGui::MenuItem("Enable")) {
                        AppsActionCookHelper::EnableAppAsync(pkg);
                    }
                    if (ImGui::MenuItem("Disable")) {
                        AppsActionCookHelper::DisableAppAsync(pkg);
                    }
                    if (ImGui::MenuItem("Open App Info")) {
                        AppsActionCookHelper::OpenAppInfoAsync(pkg);
                    }
                    if (ImGui::MenuItem("Home")) {
                        AppsActionCookHelper::GoHomeAsync();
                    }
                    if (ImGui::MenuItem("View at Playstore")) {
                        AppsActionCookHelper::ViewAtPlaystoreAsync(pkg);
                    }
                    if (ImGui::MenuItem("View at Desktop")) {
                        AppsActionCookHelper::ViewAtDesktopAsync(pkg);
                    }
                    if (ImGui::MenuItem("Find online")) {
                        AppsActionCookHelper::FindOnlineAsync(pkg);
                    }
                    if (ImGui::MenuItem("Copy")) {
                        AppsActionCookHelper::CopyPackageName(pkg);
                    }
                    if (ImGui::MenuItem("Show More")) {
                        AppsActionCookHelper::ShowMore(pkg);
                    }
                    if (ImGui::MenuItem("Unpin")) {
                        AppsActionCookHelper::UnpinApp(pkg);
                    }
                    ImGui::EndPopup();
                }
            }
        }
        
        ImGui::PopStyleColor(3);
        ImGui::Spacing();
    }
    
    // Show all apps section
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    
    if (ImGui::CollapsingHeader("All Apps", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (const auto& pkg : filteredPackages) {
            // Skip if this package is already shown in pinned section (fast lookup)
            if (pinnedSet.find(pkg) != pinnedSet.end()) {
                continue;
            }
            
            bool isSelected = (g_selectedPackage == pkg);
            
            // Use Selectable for default ImGui hover highlighting
            ImGui::Selectable(pkg.c_str(), isSelected, ImGuiSelectableFlags_None);
            
            // Right-click context menu
            if (ImGui::BeginPopupContextItem(("##ContextMenu_" + pkg).c_str())) {
                g_selectedPackage = pkg;
                
                if (ImGui::MenuItem("Open")) {
                    AppsActionCookHelper::StartAppAsync(pkg);
                }
                if (ImGui::MenuItem("Force Stop")) {
                    AppsActionCookHelper::ForceStopAppAsync(pkg);
                }
                if (ImGui::MenuItem("Restart")) {
                    AppsActionCookHelper::RestartAppAsync(pkg);
                }
                if (ImGui::MenuItem("Uninstall")) {
                    if (ShouldShowConfirmation(ConfirmationType::Uninstall)) {
                        OpenConfirmationDialog(ConfirmationType::Uninstall, pkg, 
                            [pkg]() { AppsActionCookHelper::UninstallAppAsync(pkg); });
                    } else {
                        AppsActionCookHelper::UninstallAppAsync(pkg);
                    }
                }
                if (ImGui::MenuItem("Clear Data")) {
                    if (ShouldShowConfirmation(ConfirmationType::ClearData)) {
                        OpenConfirmationDialog(ConfirmationType::ClearData, pkg, 
                            [pkg]() { AppsActionCookHelper::ClearAppDataAsync(pkg); });
                    } else {
                        AppsActionCookHelper::ClearAppDataAsync(pkg);
                    }
                }
                if (ImGui::MenuItem("Enable")) {
                    AppsActionCookHelper::EnableAppAsync(pkg);
                }
                if (ImGui::MenuItem("Disable")) {
                    AppsActionCookHelper::DisableAppAsync(pkg);
                }
                if (ImGui::MenuItem("Open App Info")) {
                    AppsActionCookHelper::OpenAppInfoAsync(pkg);
                }
                if (ImGui::MenuItem("Home")) {
                    AppsActionCookHelper::GoHomeAsync();
                }
                if (ImGui::MenuItem("View at Playstore")) {
                    AppsActionCookHelper::ViewAtPlaystoreAsync(pkg);
                }
                if (ImGui::MenuItem("View at Desktop")) {
                    AppsActionCookHelper::ViewAtDesktopAsync(pkg);
                }
                if (ImGui::MenuItem("Find online")) {
                    AppsActionCookHelper::FindOnlineAsync(pkg);
                }
                if (ImGui::MenuItem("Copy")) {
                    AppsActionCookHelper::CopyPackageName(pkg);
                }
                if (ImGui::MenuItem("Show More")) {
                    AppsActionCookHelper::ShowMore(pkg);
                }
                if (ImGui::MenuItem("Pin")) {
                    AppsActionCookHelper::PinApp(pkg);
                }
                ImGui::EndPopup();
            }
        }
    }
    
    ImGui::PopStyleColor(3);
    
    // Clear selection when no context menu is open
    if (!g_selectedPackage.empty() && !ImGui::IsPopupOpen(("##ContextMenu_" + g_selectedPackage).c_str())) {
        g_selectedPackage = "";
    }
    
    ImGui::EndChild();
}
