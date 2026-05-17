#include "lifecycle.h"
#include "lifecycle_helper.h"
#include "imgui.h"
#include "global_config.h"
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <algorithm>
#include <atomic>

// Global state for lifecycle data
static std::vector<LifecycleEntry> g_lifecycleData;
static std::vector<LifecycleEntry> g_filteredLifecycleData;
static char g_searchBuffer[256] = "";
static bool g_isFirstLoad = true;
static bool g_isLoading = false;
static std::string g_lastError = "";
static std::string g_selectedDeviceId = "";
static std::future<void> g_loadingFuture;

// Search-related state
static std::future<void> g_searchFuture;
static std::atomic<bool> g_searchCancelled{false};
static std::chrono::steady_clock::time_point g_lastSearchTime;
static std::string g_lastSearchQuery = "";
static const int SEARCH_DEBOUNCE_MS = 300;

// Column headers for the table
static const char* g_columnHeaders[] = {
    "No.", "Time", "Type", "Package Name", "Class Name", "Instance ID", "Task Root Package", "Task Root Class", "Flags"
};

void RefreshLifecycleData() {
    if (g_isLoading) return;
    
    g_isLoading = true;
    g_lastError = "";
    
    std::string currentDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (currentDeviceId.empty()) {
        g_lastError = "No device selected";
        g_isLoading = false;
        return;
    }
    
    // Check if device has changed
    if (g_selectedDeviceId != currentDeviceId) {
        g_selectedDeviceId = currentDeviceId;
        g_lifecycleData.clear();
        g_filteredLifecycleData.clear();
    }
    
    // Start background loading
    g_loadingFuture = std::async(std::launch::async, [currentDeviceId]() {
        try {
            auto data = LifecycleHelper::GetLifecycleData(currentDeviceId);
            // Update the main data on the main thread
            g_lifecycleData = std::move(data);
            g_filteredLifecycleData = LifecycleHelper::FilterLifecycleData(g_lifecycleData, std::string(g_searchBuffer));
            g_lastError = "";
        } catch (const std::exception& e) {
            g_lastError = "Error fetching lifecycle data: " + std::string(e.what());
        } catch (...) {
            g_lastError = "Unknown error occurred while fetching lifecycle data";
        }
        g_isLoading = false;
    });
}

void UpdateFilteredLifecycleData() {
    // Cancel any ongoing search
    g_searchCancelled.store(true);
    
    // Wait for current search to complete if it's running
    if (g_searchFuture.valid()) {
        g_searchFuture.wait();
    }
    
    // Start new search
    g_searchFuture = std::async(std::launch::async, []() {
        g_filteredLifecycleData = LifecycleHelper::FilterLifecycleData(g_lifecycleData, std::string(g_searchBuffer));
    });
}

void TriggerLifecycleSearchWithDebouncing() {
    if (g_searchCancelled.load()) {
        return;
    }
    UpdateFilteredLifecycleData();
}

void ShowLifecycle() {
    ImGui::Text("Lifecycle Data");
    ImGui::Separator();
    
    // Check if device is selected
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    if (selectedDeviceId.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No device selected");
        return;
    }
    
    ImGui::Spacing();
    
    // Check if background loading is complete
    if (g_loadingFuture.valid() && g_loadingFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        g_loadingFuture.get(); // This will not block since we already checked it's ready
    }
    
    // Check if background search is complete
    if (g_searchFuture.valid() && g_searchFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        g_searchFuture.get(); // This will not block since we already checked it's ready
    }
    
    // Initial load only once
    if (g_isFirstLoad && !g_isLoading) {
        RefreshLifecycleData();
        g_isFirstLoad = false;
    }
    
    // Top controls panel
    ImGui::BeginGroup();
    
    // Search input field with debouncing
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputTextWithHint("##SearchLifecycle", "Search by package name, class name, or type", g_searchBuffer, sizeof(g_searchBuffer))) {
        std::string currentQuery = std::string(g_searchBuffer);
        
        // Check if search query has changed
        if (currentQuery != g_lastSearchQuery) {
            g_lastSearchQuery = currentQuery;
            g_lastSearchTime = std::chrono::steady_clock::now();
            
            // Schedule search with debouncing
            std::thread([currentQuery]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(SEARCH_DEBOUNCE_MS));
                
                // Check if this is still the current search query
                if (currentQuery == g_lastSearchQuery) {
                    TriggerLifecycleSearchWithDebouncing();
                }
            }).detach();
        }
    }
    
    ImGui::SameLine();
    
    // Refresh button
    if (g_isLoading) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Button("Loading...");
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button("Refresh")) {
            RefreshLifecycleData();
        }
    }
    
    ImGui::SameLine();
    
    // Status info on same line as refresh button
    if (!g_lastError.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", g_lastError.c_str());
    } else {
        ImGui::Text("Total: %d (Filtered: %d)", (int)g_lifecycleData.size(), (int)g_filteredLifecycleData.size());
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Loading indicator
    if (g_isLoading) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Loading lifecycle data...");
        ImGui::Spacing();
    }
    
    // Lifecycle data table
    ImGui::BeginChild("LifecycleTable", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Use ImGui tables with virtual scrolling for optimal performance
    if (ImGui::BeginTable("LifecycleTable", IM_ARRAYSIZE(g_columnHeaders), 
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | 
                          ImGuiTableFlags_ScrollY)) {
        
        // Table header (will be sticky)
        ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Package Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Class Name", ImGuiTableColumnFlags_WidthFixed, 250.0f);
        ImGui::TableSetupColumn("Instance ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Task Root Package", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Task Root Class", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Flags", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();
        
        // Draw table rows with virtual scrolling
        if (g_filteredLifecycleData.empty()) {
            if (g_isLoading) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Loading lifecycle data...");
            } else if (!g_lastError.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to load lifecycle data");
            } else {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No lifecycle data found");
            }
        } else {
            // Use ImGui's virtual scrolling for optimal performance with large datasets
            ImGuiListClipper clipper;
            clipper.Begin(g_filteredLifecycleData.size());
            
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    const auto& entry = g_filteredLifecycleData[row];
                    
                    ImGui::TableNextRow();
                    
                    // Row number
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", row + 1);
                    
                    // Time
                    ImGui::TableSetColumnIndex(1);
                    if (entry.time.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.time.c_str());
                    }
                    
                    // Type
                    ImGui::TableSetColumnIndex(2);
                    if (entry.type.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.type.c_str());
                    }
                    
                    // Package Name
                    ImGui::TableSetColumnIndex(3);
                    if (entry.packageName.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.packageName.c_str());
                    }
                    
                    // Class Name
                    ImGui::TableSetColumnIndex(4);
                    if (entry.className.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.className.c_str());
                    }
                    
                    // Instance ID
                    ImGui::TableSetColumnIndex(5);
                    if (entry.instanceId.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.instanceId.c_str());
                    }
                    
                    // Task Root Package
                    ImGui::TableSetColumnIndex(6);
                    if (entry.taskRootPackage.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.taskRootPackage.c_str());
                    }
                    
                    // Task Root Class
                    ImGui::TableSetColumnIndex(7);
                    if (entry.taskRootClass.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.taskRootClass.c_str());
                    }
                    
                    // Flags
                    ImGui::TableSetColumnIndex(8);
                    if (entry.flags.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", entry.flags.c_str());
                    }
                }
            }
            clipper.End();
        }
        
        ImGui::EndTable();
    }
    
    ImGui::EndChild();
}
