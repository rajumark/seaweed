#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>

// Call log entry structure
struct CallLogEntry {
    std::string id;
    std::string type;
    std::string number;
    std::string name;
    std::string date;
    std::string duration;
    std::string geocodedLocation;
    std::string subject;
    std::string isCallLogPhoneAccountMigrationPending;
    std::string sourceId;
    std::string myNumber;
    std::string cloudAntispamType;
    std::string subscriptionId;
    std::string cloudAntispamTypeTag;
    std::string photoId;
    std::string postDialDigits;
    std::string callScreeningAppName;
    std::string priority;
    std::string countryIso;
    std::string forwardedCall;
    std::string sync1;
    std::string sync2;
    std::string sync3;
    std::string photoUri;
    std::string callIdDescription;
    std::string missedReason;
    std::string callIdAppName;
    std::string blockReason;
    std::string subscriptionComponentName;
    std::string addForAllUsers;
    std::string numberType;
    std::string features;
    std::string callIdName;
    std::string transcription;
    std::string phoneCallType;
    std::string callIdNuisanceConfidence;
    std::string missedCount;
    std::string lastModified;
    std::string ai;
    std::string newFlag;
    std::string simId;
    std::string contactId;
    std::string presentation;
    std::string viaNumber;
    std::string numberLabel;
    std::string normalizedNumber;
    std::string composerPhotoUri;
    std::string phoneAccountAddress;
    std::string phoneAccountHidden;
    std::string lookupUri;
    std::string voicemailUri;
    std::string matchedNumber;
    std::string callIdPackageName;
    std::string markDeleted;
    std::string transcriptionState;
    std::string dataUsage;
    std::string location;
    std::string callScreeningComponentName;
    std::string callIdDetails;
    std::string isRead;
    std::string firewallType;
};

// Forward declaration of CallLogsHelper class
class CallLogsHelper;

// Main UI function
void ShowCallLogs();
