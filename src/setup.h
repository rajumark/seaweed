#pragma once
#include <string>

bool PlatformToolsExist();
void ShowSetup(bool& setupComplete);
std::string GetADBKingPath();
bool CreateADBKingFolder();
bool DownloadPlatformTools();
void StartDownloadThread(); 