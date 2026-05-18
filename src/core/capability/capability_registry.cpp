#include "capability_registry.h"

CapabilityRegistry& CapabilityRegistry::Get() {
    static CapabilityRegistry instance;
    return instance;
}

void CapabilityRegistry::Register(const Capability& capability) {
    m_map[capability.id] = capability;
    m_list.push_back(&m_map[capability.id]);
}

Capability* CapabilityRegistry::Find(const std::string& id) {
    auto it = m_map.find(id);
    return it != m_map.end() ? &it->second : nullptr;
}

std::vector<Capability*>& CapabilityRegistry::GetAll() {
    return m_list;
}
