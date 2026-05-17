#include "force_update.h"
#include "apps_action_cook_helper.h"
#include "imgui.h"
#include <string>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/machine.h>
#endif

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/machine.h>
#endif

void OpenUrlInBrowser(const std::string& url) {
#ifdef _WIN32
    // Windows - let OS handle with default browser
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    // macOS - let OS handle with default browser
    std::string command = "open \"" + url + "\"";
    system(command.c_str());
#else
    // Linux - let OS handle with default browser
    std::string command = "xdg-open \"" + url + "\"";
    system(command.c_str());
#endif
}

std::string GetDownloadUrlForPlatform() {
#ifdef _WIN32
    return "https://github.com/rajumark/ADBKing/tree/main/WinM";
#elif defined(__APPLE__)
    // Check if it's Apple Silicon (M1/M2/M3) or Intel
    int cpuType = 0;
    size_t size = sizeof(cpuType);
    
    if (sysctlbyname("sysctl.proc_cputype", &cpuType, &size, nullptr, 0) == 0) {
        if (cpuType == CPU_TYPE_ARM64) {
            // Apple Silicon (M1/M2/M3)
            return "https://github.com/rajumark/ADBKing/tree/main/MacM";
        } else if (cpuType == CPU_TYPE_X86_64) {
            // Intel Mac
            return "https://github.com/rajumark/ADBKing/tree/main/MacIntel";
        }
    }
    
    // Fallback for Apple Silicon if detection fails
    return "https://github.com/rajumark/ADBKing/tree/main/MacM";
#else
    // Linux or other platforms
    return "https://github.com/rajumark/ADBKing/tree/main";
#endif
}

void ShowForceUpdate() {
    // Get window size for centering
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    
    // Window flags to make it non-draggable and full-window
    ImGuiWindowFlags windowFlags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
    
    // Begin the window
    if (ImGui::Begin("Force Update", nullptr, windowFlags)) {
        // Calculate center position for the content
        float contentWidth = 500.0f;
        float contentHeight = 300.0f;
        float centerX = (windowSize.x - contentWidth) * 0.5f;
        float centerY = (windowSize.y - contentHeight) * 0.5f;
        
        ImGui::SetCursorPos(ImVec2(centerX, centerY));
        
        // Create a child window for the content
        ImGui::BeginChild("ForceUpdateContent", ImVec2(contentWidth, contentHeight), true);
        
        // Center the text
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50.0f);
        
        // Title
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::SetCursorPosX((contentWidth - ImGui::CalcTextSize("Software Update Required").x) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Software Update Required");
        ImGui::PopFont();
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Warning message
        ImGui::SetCursorPosX((contentWidth - ImGui::CalcTextSize("Please update the software. This is outdated version").x) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Please update the software. This is outdated version");
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Additional info
        ImGui::SetCursorPosX((contentWidth - ImGui::CalcTextSize("A new version is available with important updates and bug fixes.").x) * 0.5f);
        ImGui::Text("A new version is available with important updates and bug fixes.");
        
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Platform-specific info
        std::string platformInfo = "Your platform: ";
#ifdef _WIN32
        platformInfo += "Windows";
#elif defined(__APPLE__)
        int cpuType = 0;
        size_t size = sizeof(cpuType);
        if (sysctlbyname("sysctl.proc_cputype", &cpuType, &size, nullptr, 0) == 0) {
            if (cpuType == CPU_TYPE_ARM64) {
                platformInfo += "macOS (Apple Silicon)";
            } else if (cpuType == CPU_TYPE_X86_64) {
                platformInfo += "macOS (Intel)";
            } else {
                platformInfo += "macOS (Unknown)";
            }
        } else {
            platformInfo += "macOS (Unknown)";
        }
#else
        platformInfo += "Linux/Other";
#endif
        
        ImGui::SetCursorPosX((contentWidth - ImGui::CalcTextSize(platformInfo.c_str()).x) * 0.5f);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", platformInfo.c_str());
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        
        // Download button
        ImGui::SetCursorPosX((contentWidth - 120.0f) * 0.5f);
        if (ImGui::Button("Download", ImVec2(120.0f, 40.0f))) {
            std::string downloadUrl = GetDownloadUrlForPlatform();
            OpenUrlInBrowser(downloadUrl);
        }
        
        ImGui::EndChild();
    }
    
    ImGui::End();
}
