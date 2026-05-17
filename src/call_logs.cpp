#include "call_logs.h"
#include "call_logs_helper.h"
#include "imgui.h"
#include "global_config.h"
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <algorithm>
#include <atomic>

// Global state for call logs
static std::vector<CallLogEntry> g_callLogs;
static std::vector<CallLogEntry> g_filteredCallLogs;
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
    "No.", "Type", "Number", "Name", "Date", "Duration", "Location"
};

void RefreshCallLogs() {
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
        g_callLogs.clear();
        g_filteredCallLogs.clear();
    }
    
    // Start background loading
    g_loadingFuture = std::async(std::launch::async, [currentDeviceId]() {
        try {
            auto logs = CallLogsHelper::GetCallLogs(currentDeviceId);
            // Update the main data on the main thread
            g_callLogs = std::move(logs);
            g_filteredCallLogs = CallLogsHelper::FilterCallLogs(g_callLogs, std::string(g_searchBuffer));
            g_lastError = "";
        } catch (const std::exception& e) {
            g_lastError = "Error fetching call logs: " + std::string(e.what());
        } catch (...) {
            g_lastError = "Unknown error occurred while fetching call logs";
        }
        g_isLoading = false;
    });
}

void UpdateFilteredCallLogs() {
    // Cancel any ongoing search
    g_searchCancelled = true;
    
    // Wait for previous search to complete if it's running
    if (g_searchFuture.valid()) {
        g_searchFuture.wait();
    }
    
    // Reset cancellation flag
    g_searchCancelled = false;
    
    // Start background search
    g_searchFuture = std::async(std::launch::async, []() {
        std::string currentQuery = std::string(g_searchBuffer);
        
        // Check if search was cancelled
        if (g_searchCancelled) {
            return;
        }
        
        // Perform the filtering
        auto filtered = CallLogsHelper::FilterCallLogs(g_callLogs, currentQuery);
        
        // Check again if search was cancelled before updating
        if (!g_searchCancelled) {
            g_filteredCallLogs = std::move(filtered);
        }
    });
}

void TriggerSearchWithDebouncing() {
    std::string currentQuery = std::string(g_searchBuffer);
    
    // Cancel any ongoing search
    g_searchCancelled = true;
    
    // Wait for previous search to complete if it's running
    if (g_searchFuture.valid()) {
        g_searchFuture.wait();
    }
    
    // Reset cancellation flag
    g_searchCancelled = false;
    
    // Start background search with debouncing
    g_searchFuture = std::async(std::launch::async, [currentQuery]() {
        // Check if search was cancelled
        if (g_searchCancelled) {
            return;
        }
        
        // Perform the filtering
        auto filtered = CallLogsHelper::FilterCallLogs(g_callLogs, currentQuery);
        
        // Check again if search was cancelled before updating
        if (!g_searchCancelled) {
            g_filteredCallLogs = std::move(filtered);
        }
    });
}

void ShowCallLogs() {
    ImGui::Text("Call Logs");
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
        RefreshCallLogs();
        g_isFirstLoad = false;
    }
    
    // Top controls panel
    ImGui::BeginGroup();
    
    // Search input field with debouncing
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputTextWithHint("##SearchCallLogs", "Search by name or number", g_searchBuffer, sizeof(g_searchBuffer))) {
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
                    TriggerSearchWithDebouncing();
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
            RefreshCallLogs();
        }
    }
    
    ImGui::SameLine();
    
    // Status info on same line as refresh button
    if (!g_lastError.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", g_lastError.c_str());
    } else {
        ImGui::Text("Total: %d (Filtered: %d)", (int)g_callLogs.size(), (int)g_filteredCallLogs.size());
    }
    
    ImGui::EndGroup();
    
    ImGui::Spacing();
    
    // Loading indicator
    if (g_isLoading) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Loading call logs...");
        ImGui::Spacing();
    }
    
    // Call logs table
    ImGui::BeginChild("CallLogsTable", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Use ImGui tables with virtual scrolling for optimal performance
    if (ImGui::BeginTable("CallLogsTable", IM_ARRAYSIZE(g_columnHeaders), 
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | 
                          ImGuiTableFlags_ScrollY)) {
        
        // Table header (will be sticky)
        ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Number", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Location", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableHeadersRow();
        
        // Draw table rows with virtual scrolling
        if (g_filteredCallLogs.empty()) {
            if (g_isLoading) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Loading call logs...");
            } else if (!g_lastError.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to load call logs");
            } else {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No call logs found");
            }
        } else {
            // Use ImGui's virtual scrolling for optimal performance with large datasets
            ImGuiListClipper clipper;
            clipper.Begin(g_filteredCallLogs.size());
            
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    const auto& log = g_filteredCallLogs[row];
                    
                    ImGui::TableNextRow();
                    
                    // Row number
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", row + 1);
                    
                    // Call type
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", log.type.c_str());
                    
                    // Phone number
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", log.number.c_str());
                    
                    // Contact name
                    ImGui::TableSetColumnIndex(3);
                    if (log.name.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Unknown");
                    } else {
                        ImGui::Text("%s", log.name.c_str());
                    }
                    
                    // Date
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", log.date.c_str());
                    
                    // Duration
                    ImGui::TableSetColumnIndex(5);
                    if (log.duration.empty() || log.duration == "0") {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "0s");
                    } else {
                        try {
                            int duration = std::stoi(log.duration);
                            int minutes = duration / 60;
                            int seconds = duration % 60;
                            if (minutes > 0) {
                                ImGui::Text("%dm %ds", minutes, seconds);
                            } else {
                                ImGui::Text("%ds", seconds);
                            }
                        } catch (...) {
                            ImGui::Text("%s", log.duration.c_str());
                        }
                    }
                    
                    // Location
                    ImGui::TableSetColumnIndex(6);
                    if (log.geocodedLocation.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "-");
                    } else {
                        ImGui::Text("%s", log.geocodedLocation.c_str());
                    }
                }
            }
            clipper.End();
        }
        
        ImGui::EndTable();
    }
    
    ImGui::EndChild();
}
