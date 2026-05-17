#include "call_logs_helper.h"
#include "call_logs.h"
#include "global_config.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iomanip>

std::vector<CallLogEntry> CallLogsHelper::GetCallLogs(const std::string& deviceId) {
    std::vector<CallLogEntry> callLogs;
    
    if (deviceId.empty()) {
        return callLogs;
    }
    
    std::string adbPath = GlobalConfig::GetADBPath();
    if (adbPath.empty()) {
        return callLogs;
    }
    
    // Build the ADB command to query call logs
    std::string command = "\"" + adbPath + "\" -s " + deviceId + " shell content query --uri content://call_log/calls";
    
    // Execute the command
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return callLogs;
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
        if (!line.empty() && line.find("Row:") != std::string::npos) {
            CallLogEntry entry = ParseCallData(line);
            if (!entry.id.empty()) {
                callLogs.push_back(entry);
            }
        }
    }
    
    // Sort by ID in descending order (most recent first)
    std::sort(callLogs.begin(), callLogs.end(), [](const CallLogEntry& a, const CallLogEntry& b) {
        try {
            int idA = std::stoi(a.id);
            int idB = std::stoi(b.id);
            return idA > idB;
        } catch (...) {
            return false;
        }
    });
    
    return callLogs;
}

CallLogEntry CallLogsHelper::ParseCallData(const std::string& input) {
    CallLogEntry entry;
    
    if (input.empty() || input.find("Row:") == std::string::npos) {
        return entry;
    }
    
    // Remove "Row: " prefix and trim
    std::string cleanInput = input.substr(input.find("Row:") + 4);
    
    // Split by comma and parse key-value pairs
    std::istringstream iss(cleanInput);
    std::string field;
    
    while (std::getline(iss, field, ',')) {
        // Trim whitespace
        field.erase(0, field.find_first_not_of(" \t"));
        field.erase(field.find_last_not_of(" \t") + 1);
        
        size_t equalPos = field.find('=');
        if (equalPos != std::string::npos) {
            std::string key = field.substr(0, equalPos);
            std::string value = field.substr(equalPos + 1);
            
            // Handle multi-value fields (some fields might have commas in their values)
            // Look ahead for additional values that don't contain '='
            std::string nextField;
            while (std::getline(iss, nextField, ',')) {
                nextField.erase(0, nextField.find_first_not_of(" \t"));
                nextField.erase(nextField.find_last_not_of(" \t") + 1);
                
                if (nextField.find('=') == std::string::npos && !nextField.empty()) {
                    value += ", " + nextField;
                } else {
                    // Put the field back for next iteration
                    iss.seekg(-static_cast<long>(nextField.length() + 1), std::ios::cur);
                    break;
                }
            }
            
            // Map the key to the appropriate field
            if (key == "_id") entry.id = value;
            else if (key == "type") entry.type = GetCallTypeDescription(std::stoi(value));
            else if (key == "number") entry.number = value;
            else if (key == "name") entry.name = value;
            else if (key == "date") entry.date = ConvertTimestampToDate(value);
            else if (key == "duration") entry.duration = value;
            else if (key == "geocoded_location") entry.geocodedLocation = value;
            else if (key == "subject") entry.subject = value;
            else if (key == "is_call_log_phone_account_migration_pending") entry.isCallLogPhoneAccountMigrationPending = value;
            else if (key == "source_id") entry.sourceId = value;
            else if (key == "my_number") entry.myNumber = value;
            else if (key == "cloud_antispam_type") entry.cloudAntispamType = value;
            else if (key == "subscription_id") entry.subscriptionId = value;
            else if (key == "cloud_antispam_type_tag") entry.cloudAntispamTypeTag = value;
            else if (key == "photo_id") entry.photoId = value;
            else if (key == "post_dial_digits") entry.postDialDigits = value;
            else if (key == "call_screening_app_name") entry.callScreeningAppName = value;
            else if (key == "priority") entry.priority = value;
            else if (key == "countryiso") entry.countryIso = value;
            else if (key == "forwarded_call") entry.forwardedCall = value;
            else if (key == "sync_1") entry.sync1 = value;
            else if (key == "sync_2") entry.sync2 = value;
            else if (key == "sync_3") entry.sync3 = value;
            else if (key == "photo_uri") entry.photoUri = value;
            else if (key == "call_id_description") entry.callIdDescription = value;
            else if (key == "missed_reason") entry.missedReason = value;
            else if (key == "call_id_app_name") entry.callIdAppName = value;
            else if (key == "block_reason") entry.blockReason = value;
            else if (key == "subscription_component_name") entry.subscriptionComponentName = value;
            else if (key == "add_for_all_users") entry.addForAllUsers = value;
            else if (key == "numbertype") entry.numberType = value;
            else if (key == "features") entry.features = value;
            else if (key == "call_id_name") entry.callIdName = value;
            else if (key == "transcription") entry.transcription = value;
            else if (key == "phone_call_type") entry.phoneCallType = value;
            else if (key == "call_id_nuisance_confidence") entry.callIdNuisanceConfidence = value;
            else if (key == "missed_count") entry.missedCount = value;
            else if (key == "last_modified") entry.lastModified = value;
            else if (key == "ai") entry.ai = value;
            else if (key == "new") entry.newFlag = value;
            else if (key == "simid") entry.simId = value;
            else if (key == "contact_id") entry.contactId = value;
            else if (key == "presentation") entry.presentation = value;
            else if (key == "via_number") entry.viaNumber = value;
            else if (key == "number_type") entry.numberType = value;
            else if (key == "numberlabel") entry.numberLabel = value;
            else if (key == "normalized_number") entry.normalizedNumber = value;
            else if (key == "composer_photo_uri") entry.composerPhotoUri = value;
            else if (key == "phone_account_address") entry.phoneAccountAddress = value;
            else if (key == "phone_account_hidden") entry.phoneAccountHidden = value;
            else if (key == "lookup_uri") entry.lookupUri = value;
            else if (key == "voicemail_uri") entry.voicemailUri = value;
            else if (key == "matched_number") entry.matchedNumber = value;
            else if (key == "call_id_package_name") entry.callIdPackageName = value;
            else if (key == "mark_deleted") entry.markDeleted = value;
            else if (key == "transcription_state") entry.transcriptionState = value;
            else if (key == "data_usage") entry.dataUsage = value;
            else if (key == "location") entry.location = value;
            else if (key == "call_screening_component_name") entry.callScreeningComponentName = value;
            else if (key == "call_id_details") entry.callIdDetails = value;
            else if (key == "is_read") entry.isRead = value;
            else if (key == "firewalltype") entry.firewallType = value;
        }
    }
    
    return entry;
}

std::string CallLogsHelper::GetCallTypeDescription(int callType) {
    switch (callType) {
        case 1: return "Incoming Call";
        case 2: return "Outgoing Call";
        case 3: return "Missed Call";
        case 4: return "Rejected Call";
        case 5: return "Blocked Call";
        case 6: return "Voicemail Call";
        default: return "Unknown Call Type(" + std::to_string(callType) + ")";
    }
}

std::string CallLogsHelper::ConvertTimestampToDate(const std::string& timestamp) {
    try {
        long long timestampValue = std::stoll(timestamp);
        std::time_t time = timestampValue / 1000; // Convert milliseconds to seconds
        
        std::tm* tm = std::localtime(&time);
        if (tm) {
            std::ostringstream oss;
            oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }
    } catch (...) {
        // If conversion fails, return the original timestamp
    }
    return timestamp;
}

std::vector<CallLogEntry> CallLogsHelper::FilterCallLogs(const std::vector<CallLogEntry>& logs, const std::string& searchQuery) {
    if (searchQuery.empty()) {
        return logs;
    }
    
    std::vector<CallLogEntry> filteredLogs;
    std::string lowerQuery = searchQuery;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
    
    for (const auto& log : logs) {
        std::string lowerName = log.name;
        std::string lowerNumber = log.number;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerNumber.begin(), lowerNumber.end(), lowerNumber.begin(), ::tolower);
        
        if (lowerName.find(lowerQuery) != std::string::npos || 
            lowerNumber.find(lowerQuery) != std::string::npos) {
            filteredLogs.push_back(log);
        }
    }
    
    return filteredLogs;
}

std::vector<std::string> CallLogsHelper::GetValidCallLogColumns() {
    return {
        "type",
        "number", 
        "name",
        "date",
        "duration",
        "geocoded_location"
    };
}
