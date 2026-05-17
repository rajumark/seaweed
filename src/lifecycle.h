#pragma once
#include <string>
#include <vector>

// Lifecycle entry structure
struct LifecycleEntry {
    std::string time;
    std::string type;
    std::string packageName;
    std::string className;
    std::string instanceId;
    std::string taskRootPackage;
    std::string taskRootClass;
    std::string flags;
};

// Forward declaration of LifecycleHelper class
class LifecycleHelper;

// Main UI function
void ShowLifecycle();
