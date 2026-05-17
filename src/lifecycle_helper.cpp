#include "lifecycle_helper.h"
#include "lifecycle.h"
#include "global_config.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstring>

std::vector<LifecycleEntry> LifecycleHelper::GetLifecycleData(const std::string& deviceId) {
    std::vector<LifecycleEntry> lifecycleData;
    
    if (deviceId.empty()) {
        return lifecycleData;
    }
    
    std::string adbPath = GlobalConfig::GetADBPath();
    if (adbPath.empty()) {
        return lifecycleData;
    }
    
    // Build the ADB command to query lifecycle data using dumpsys usagestats
    std::string command = "\"" + adbPath + "\" -s " + deviceId + " shell dumpsys usagestats";
    
    // Execute the command
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return lifecycleData;
    }
    
    char buffer[4096];
    std::string result;
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    
    // Parse the result line by line
    std::istringstream iss(result);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line.find("time=") != std::string::npos) {
            LifecycleEntry entry = ParseLifecycleData(line);
            if (!entry.time.empty() && !entry.type.empty()) {
                lifecycleData.push_back(entry);
            }
        }
    }
    
    // Sort by time in descending order (most recent first)
    std::sort(lifecycleData.begin(), lifecycleData.end(), [](const LifecycleEntry& a, const LifecycleEntry& b) {
        return a.time > b.time;
    });
    
    return lifecycleData;
}

LifecycleEntry LifecycleHelper::ParseLifecycleData(const std::string& input) {
    LifecycleEntry entry;
    
    if (input.empty()) {
        return entry;
    }
    
    // Extract time first
    entry.time = ExtractTimeString(input);
    
    // Split the input string by spaces while preserving key-value pairs
    std::istringstream iss(input);
    std::string token;
    
    while (iss >> token) {
        size_t equalPos = token.find('=');
        if (equalPos != std::string::npos) {
            std::string key = token.substr(0, equalPos);
            std::string value = token.substr(equalPos + 1);
            
            // Remove quotes if present
            if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            // Map the key to the appropriate field
            if (key == "type") entry.type = value;
            else if (key == "package") entry.packageName = value;
            else if (key == "class") entry.className = value;
            else if (key == "instanceId") entry.instanceId = value;
            else if (key == "taskRootPackage") entry.taskRootPackage = value;
            else if (key == "taskRootClass") entry.taskRootClass = value;
            else if (key == "flags") entry.flags = value;
        }
    }
    
    return entry;
}

std::string LifecycleHelper::ExtractTimeString(const std::string& input) {
    size_t startIndex = input.find("time=\"");
    if (startIndex == std::string::npos) {
        return "";
    }
    
    startIndex += 6; // Length of 'time="'
    size_t endIndex = input.find("\"", startIndex);
    
    if (endIndex == std::string::npos) {
        return "";
    }
    
    return input.substr(startIndex, endIndex - startIndex);
}

std::vector<LifecycleEntry> LifecycleHelper::FilterLifecycleData(const std::vector<LifecycleEntry>& data, const std::string& searchQuery) {
    if (searchQuery.empty()) {
        return data;
    }
    
    std::vector<LifecycleEntry> filteredData;
    std::string lowerQuery = searchQuery;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& entry : data) {
        std::string lowerPackageName = entry.packageName;
        std::string lowerClassName = entry.className;
        std::string lowerType = entry.type;
        
        std::transform(lowerPackageName.begin(), lowerPackageName.end(), lowerPackageName.begin(), ::tolower);
        std::transform(lowerClassName.begin(), lowerClassName.end(), lowerClassName.begin(), ::tolower);
        std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
        
        if (lowerPackageName.find(lowerQuery) != std::string::npos || 
            lowerClassName.find(lowerQuery) != std::string::npos ||
            lowerType.find(lowerQuery) != std::string::npos) {
            filteredData.push_back(entry);
        }
    }
    
    return filteredData;
}
