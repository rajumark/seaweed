#pragma once
#include <string>
#include <vector>

// Forward declaration of CallLogEntry
struct CallLogEntry;

// Call logs helper class for ADB operations
class CallLogsHelper {
public:
    // Get all call logs from the specified device
    static std::vector<CallLogEntry> GetCallLogs(const std::string& deviceId);
    
    // Convert call type integer to human-readable description
    static std::string GetCallTypeDescription(int callType);
    
    // Convert timestamp to readable date format
    static std::string ConvertTimestampToDate(const std::string& timestamp);
    
    // Filter call logs based on search query
    static std::vector<CallLogEntry> FilterCallLogs(const std::vector<CallLogEntry>& logs, const std::string& searchQuery);
    
private:
    // Parse raw call data from ADB output
    static CallLogEntry ParseCallData(const std::string& input);
    
    // Get list of valid call log column names
    static std::vector<std::string> GetValidCallLogColumns();
};
