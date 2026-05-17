#pragma once
#include <string>
#include <vector>

// Forward declaration of LifecycleEntry
struct LifecycleEntry;

// Lifecycle helper class for ADB operations
class LifecycleHelper {
public:
    // Get all lifecycle data from the specified device
    static std::vector<LifecycleEntry> GetLifecycleData(const std::string& deviceId);
    
    // Filter lifecycle data based on search query
    static std::vector<LifecycleEntry> FilterLifecycleData(const std::vector<LifecycleEntry>& data, const std::string& searchQuery);
    
private:
    // Parse raw lifecycle data from ADB output
    static LifecycleEntry ParseLifecycleData(const std::string& input);
    
    // Extract time string from input
    static std::string ExtractTimeString(const std::string& input);
};
