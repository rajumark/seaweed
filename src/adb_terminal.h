#pragma once

#include <string>
#include <vector>

// Function to show the ADB Terminal screen
void ShowADBTerminal();

// Function to cleanup ADB terminal resources
void CleanupADBTerminal();

// State variables for ADB terminal
extern std::string g_adbCommand;
extern std::string g_adbOutput;
extern bool g_commandRunning;
