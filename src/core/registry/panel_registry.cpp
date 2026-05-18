#include "panel_registry.h"
#include <algorithm>

PanelRegistry& PanelRegistry::Get() {
    static PanelRegistry instance;
    return instance;
}

void PanelRegistry::Register(const Panel& panel) {
    m_panels.push_back(panel);
}

std::vector<Panel>& PanelRegistry::GetAll() {
    return m_panels;
}

Panel* PanelRegistry::Find(const std::string& id) {
    for (auto& p : m_panels)
        if (p.id == id) return &p;
    return nullptr;
}

void PanelRegistry::Toggle(const std::string& id) {
    Panel* p = Find(id);
    if (p && p->show) *p->show = !*p->show;
}

void PanelRegistry::CloseAll() {
    for (auto& p : m_panels)
        if (p.show) *p.show = false;
}

bool PanelRegistry::AnyOpen() const {
    for (const auto& p : m_panels)
        if (p.show && *p.show) return true;
    return false;
}

void PanelRegistry::DrawAll() {
    for (auto& p : m_panels)
        if (p.show && *p.show)
            p.draw();
}
