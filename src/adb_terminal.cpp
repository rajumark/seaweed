#include "adb_terminal.h"
#include "global_config.h"
#include "device_manager.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <mutex>

// Global state variables for ADB terminal
std::string g_adbCommand = "adb devices";
std::string g_adbOutput;
bool g_commandRunning = false;
std::atomic<bool> g_cancelCommand{false};
std::thread g_commandThread;
std::mutex g_outputMutex;
std::atomic<bool> g_isStreaming{false};

// Function to execute ADB command with streaming support
std::string ExecuteADBCommandWithStreaming(const std::string& command) {
    std::string adbPath = GlobalConfig::GetADBPath();
    if (adbPath.empty()) {
        return "Error: ADB path not configured\nPlease configure ADB path in settings.";
    }
    
    if (command.empty()) {
        return "Error: No command entered\nPlease type a command to execute.";
    }
    
    // Construct the full command - check if user already included "adb"
    std::string fullCommand;
    if (command.substr(0, 3) == "adb") {
        // User typed full command like "adb devices", so just use the command as-is
        fullCommand = adbPath + " " + command.substr(4); // Remove "adb " and add our path
    } else {
        // User typed just the command like "devices", so prepend our ADB path
        fullCommand = adbPath + " " + command;
    }
    
    std::string result;
    
    // Use _popen on Windows - capture both stdout and stderr by redirecting stderr to stdout
    #ifdef _WIN32
    std::string redirectCommand = fullCommand + " 2>&1";
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(redirectCommand.c_str(), "r"), _pclose);
    if (!pipe) {
        return "Error: Failed to execute command\nCommand: " + fullCommand + "\nReason: Could not create process";
    }
    #else
    std::string redirectCommand = fullCommand + " 2>&1";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(redirectCommand.c_str(), "r"), pclose);
    #endif
    
    // Capture output
    std::array<char, 128> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
        
        // Check if user wants to cancel
        if (g_cancelCommand.load()) {
            result += "\n\n[Command cancelled by user]\n";
            break;
        }
    }
    
    // If no output was captured, provide feedback
    if (result.empty()) {
        result = "Command executed successfully with no output.\nCommand: " + fullCommand;
    }
    
    return result;
}

// Function to start streaming command in background
void StartStreamingCommand(const std::string& command) {
    // Clean up any existing thread
    if (g_commandThread.joinable()) {
        g_commandThread.detach();
    }
    
    g_cancelCommand.store(false);
    g_isStreaming.store(true);
    g_commandRunning = true;
    
    // Clear previous output for streaming commands
    {
        std::lock_guard<std::mutex> lock(g_outputMutex);
        g_adbOutput.clear();
    }
    
    g_commandThread = std::thread([command]() {
        std::string adbPath = GlobalConfig::GetADBPath();
        if (adbPath.empty()) {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            g_adbOutput = "Error: ADB path not configured\nPlease configure ADB path in settings.";
            g_commandRunning = false;
            g_isStreaming.store(false);
            return;
        }
        
        // Construct the full command
        std::string fullCommand;
        if (command.substr(0, 3) == "adb") {
            fullCommand = adbPath + " " + command.substr(4);
        } else {
            fullCommand = adbPath + " " + command;
        }
        
        std::string redirectCommand = fullCommand + " 2>&1";
        
        #ifdef _WIN32
        FILE* pipe = _popen(redirectCommand.c_str(), "r");
        #else
        FILE* pipe = popen(redirectCommand.c_str(), "r");
        #endif
        
        if (!pipe) {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            g_adbOutput = "Error: Failed to execute command\nCommand: " + fullCommand + "\nReason: Could not create process";
            g_commandRunning = false;
            g_isStreaming.store(false);
            return;
        }
        
        std::array<char, 128> buffer;
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr && !g_cancelCommand.load()) {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            g_adbOutput += buffer.data();
        }
        
        #ifdef _WIN32
        _pclose(pipe);
        #else
        pclose(pipe);
        #endif
        
        if (g_cancelCommand.load()) {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            g_adbOutput += "\n\n[Command cancelled by user]\n";
        }
        
        g_commandRunning = false;
        g_isStreaming.store(false);
    });
}

// Function to cleanup ADB terminal resources
void CleanupADBTerminal() {
    g_cancelCommand.store(true);
    
    if (g_commandThread.joinable()) {
        g_commandThread.join();
    }
    
    g_commandRunning = false;
    g_isStreaming.store(false);
}

// Legacy function for backward compatibility
std::string ExecuteADBCommand(const std::string& command) {
    return ExecuteADBCommandWithStreaming(command);
}

void ShowADBTerminal() {
    // Get current device information
    std::vector<DeviceInfo> devices = GlobalConfig::GetDevices();
    std::string selectedDeviceId = GlobalConfig::GetSelectedDeviceId();
    std::string adbPath = GlobalConfig::GetADBPath();
    
    // Display title
    ImGui::Text("ADB Terminal");
    ImGui::Separator();
    ImGui::Spacing();
    
    // Command input and buttons row
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f); // 40% of parent window width
    
    // Use a char buffer for InputText
    static char commandBuffer[1024] = "";
    
    if (ImGui::InputTextWithHint("##CommandInput", "adb shell ...", commandBuffer, sizeof(commandBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Execute command when Enter is pressed
        g_adbCommand = commandBuffer;
        if (!g_adbCommand.empty()) {
            // Check if this is a streaming command
            std::string lowerCommand = g_adbCommand;
            std::transform(lowerCommand.begin(), lowerCommand.end(), lowerCommand.begin(), ::tolower);
            
            if (lowerCommand.find("logcat") != std::string::npos || 
                lowerCommand.find("shell top") != std::string::npos ||
                lowerCommand.find("monkey") != std::string::npos) {
                // Use streaming for long-running commands
                StartStreamingCommand(g_adbCommand);
            } else {
                // Use regular execution for short commands
                g_commandRunning = true;
                g_cancelCommand.store(false);
                g_adbOutput = ExecuteADBCommandWithStreaming(g_adbCommand);
                g_commandRunning = false;
            }
        } else {
            g_adbOutput = "Error: No command entered\nPlease type a command to execute.";
        }
    }
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    
    // Run button
    if (ImGui::Button("Run", ImVec2(80, 0))) {
        g_adbCommand = commandBuffer;
        if (!g_adbCommand.empty()) {
            // Check if this is a streaming command
            std::string lowerCommand = g_adbCommand;
            std::transform(lowerCommand.begin(), lowerCommand.end(), lowerCommand.begin(), ::tolower);
            
            if (lowerCommand.find("logcat") != std::string::npos || 
                lowerCommand.find("shell top") != std::string::npos ||
                lowerCommand.find("monkey") != std::string::npos) {
                // Use streaming for long-running commands
                StartStreamingCommand(g_adbCommand);
            } else {
                // Use regular execution for short commands
                g_commandRunning = true;
                g_cancelCommand.store(false);
                g_adbOutput = ExecuteADBCommandWithStreaming(g_adbCommand);
                g_commandRunning = false;
            }
        } else {
            g_adbOutput = "Error: No command entered\nPlease type a command to execute.";
        }
    }
    
    ImGui::SameLine();
    
    // Stop button (only visible when streaming command is running)
    if (g_isStreaming.load() || g_commandRunning) {
        if (ImGui::Button("Stop", ImVec2(80, 0))) {
            g_cancelCommand.store(true);
            // Force stop the streaming thread if it's running
            if (g_commandThread.joinable()) {
                g_commandThread.detach(); // Detach to avoid blocking
            }
            g_commandRunning = false;
            g_isStreaming.store(false);
        }
        ImGui::SameLine();
    }
    
    // Clear button
    if (ImGui::Button("Clear", ImVec2(80, 0))) {
        g_adbOutput.clear();
    }
    
    ImGui::Spacing();
    
    // Output display area
    ImGui::Text("Output:");
    
    // Show command status
    if (g_commandRunning) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), " [Running...]");
    }
    
    ImGui::Separator();
    
    // Create a child window for the output area with scrolling
    ImGui::BeginChild("##OutputArea", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    // Display the output text
    if (!g_adbOutput.empty()) {
        // Use InputTextMultiline for selectable text
        std::string displayOutput;
        {
            std::lock_guard<std::mutex> lock(g_outputMutex);
            displayOutput = g_adbOutput;
        }
        
        ImGui::InputTextMultiline("##OutputText", const_cast<char*>(displayOutput.c_str()), 
                                 displayOutput.length() + 1, ImVec2(-1, -1), 
                                 ImGuiInputTextFlags_ReadOnly);
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 0.5f), "No output. Enter a command and click Run.");
    }
    
    // Auto-scroll to bottom if new output is added
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
}
