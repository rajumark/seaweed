#pragma once

#include <string>
#include <vector>

// Pinned apps store API.
// Manages a list of pinned package names stored in a separate text file.
// This module handles pinning/unpinning apps and retrieving the pinned list.
namespace AppsPinnedStore {
    // Pins a package name (adds it to the pinned list)
    // Returns true on success.
    bool PinPackage(const std::string& packageName);

    // Unpins a package name (removes it from the pinned list)
    // Returns true if the package was pinned and is now removed.
    bool UnpinPackage(const std::string& packageName);

    // Checks if a package is pinned
    // Returns true if the package is in the pinned list.
    bool IsPackagePinned(const std::string& packageName);

    // Gets all pinned package names
    // Returns a vector of all currently pinned package names.
    std::vector<std::string> GetPinnedPackages();

    // Gets the count of pinned packages
    // Returns the number of currently pinned packages.
    size_t GetPinnedCount();
    
    // Clear the internal cache (call this when you want to force a reload)
    void ClearCache();
}
