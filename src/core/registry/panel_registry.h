#pragma once
#include <string>
#include <vector>
#include <functional>

struct Panel {
    std::string id;
    std::string title;
    std::string category;
    std::function<void()> draw;
    bool* show;
    std::vector<std::string> keywords;
};

class PanelRegistry {
public:
    static PanelRegistry& Get();

    void Register(const Panel& panel);

    std::vector<Panel>& GetAll();
    Panel* Find(const std::string& id);
    void Toggle(const std::string& id);
    void CloseAll();
    bool AnyOpen() const;
    void DrawAll();

    void SaveState(const std::string& filepath) const;
    void LoadState(const std::string& filepath);

private:
    PanelRegistry() = default;
    std::vector<Panel> m_panels;
};
