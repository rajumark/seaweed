#include "panel_registry.h"
#include <algorithm>
#include <fstream>
#include <sstream>

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

void PanelRegistry::SaveState(const std::string& filepath) const {
    std::ofstream out(filepath, std::ios::trunc);
    if (!out.is_open()) return;
    out << "# ADBKing window state\n";
    for (const auto& p : m_panels) {
        if (p.show)
            out << p.id << '=' << (*p.show ? '1' : '0') << '\n';
    }
}

void PanelRegistry::LoadState(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string id = line.substr(0, eq);
        bool value = (line.size() > eq + 1 && line[eq + 1] == '1');
        Panel* p = Find(id);
        if (p && p->show) *p->show = value;
    }
}
