#pragma once
#include "capability.h"
#include <vector>
#include <unordered_map>

class CapabilityRegistry {
public:
    static CapabilityRegistry& Get();

    void Register(const Capability& capability);
    Capability* Find(const std::string& id);
    std::vector<Capability*>& GetAll();

private:
    CapabilityRegistry() = default;
    std::vector<Capability*> m_list;
    std::unordered_map<std::string, Capability> m_map;
};
