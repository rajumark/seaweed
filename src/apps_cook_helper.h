#pragma once

#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace AppsCookHelper {

// Load packages list from device (async version)
void LoadPackagesListAsync();

// Check if background operation is in progress
bool IsLoading();

// Load packages list from device (sync version - for immediate use)
void LoadPackagesList();

// Filter management
void SetFilter(int filter);
int GetFilter();

// Data access (thread-safe)
const std::vector<std::string>& GetPackages();
const std::string& GetLastError();

// Search and filtering
std::vector<std::string> GetFilteredPackages(const std::string& searchTerm);

// Cleanup background thread
void Cleanup();

} // namespace AppsCookHelper
