#pragma once
#include <string>

void ShowSetup(bool& setupComplete);
std::string GetADBKingPath();
bool CreateADBKingFolder();
bool DownloadPlatformTools();
void StartDownloadThread(); 