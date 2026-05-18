#pragma once
#include <string>
#include <vector>
#include <functional>

struct CommandContext {
    std::string deviceId;
};

enum class CachePolicy {
    None,
    Fixed,
    UntilChange
};

struct Capability {
    std::string id;
    std::string name;
    std::string category;
    std::string commandTemplate;
    CachePolicy cachePolicy = CachePolicy::Fixed;
    int cacheTtlMs = 30000;
    std::function<std::vector<std::string>(const std::string&)> lineParser;
    std::vector<std::string> keywords;
    bool requiresDevice = true;
};
