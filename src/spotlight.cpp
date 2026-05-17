#include "spotlight.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <vector>

static std::vector<SpotlightItem> g_items;
static bool g_open = false;
static char g_search[128] = "";
static int g_selected = 0;
static bool g_focusSearch = false;
static std::string g_prevSearch;
static Uint32 g_lastShift = 0;

struct HistoryEntry {
    std::string label;
    std::string category;
};
static std::vector<HistoryEntry> g_history;
static const int MAX_HISTORY = 10;

void RegisterSpotlightItem(const SpotlightItem& item) {
    for (const auto& e : g_items) {
        if (e.label == item.label && e.category == item.category)
            return;
    }
    g_items.push_back(item);
}

bool SpotlightHandleSDLEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        auto sym = event.key.keysym.sym;
        if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) {
            Uint32 now = SDL_GetTicks();
            if (now - g_lastShift < 400 && now - g_lastShift > 50) {
                g_open = !g_open;
                if (g_open) {
                    g_search[0] = '\0';
                    g_selected = 0;
                    g_focusSearch = true;
                }
                g_lastShift = 0;
                return true;
            }
            g_lastShift = now;
        } else {
            g_lastShift = 0;
        }
    }
    return false;
}

static void AddToHistory(const SpotlightItem& item) {
    for (auto it = g_history.begin(); it != g_history.end(); ++it) {
        if (it->label == item.label && it->category == item.category) {
            g_history.erase(it);
            break;
        }
    }
    g_history.insert(g_history.begin(), {item.label, item.category});
    if (g_history.size() > MAX_HISTORY)
        g_history.pop_back();
}

static void ActivateItem(const SpotlightItem& item) {
    if (item.toggleItem) {
        bool alreadyOpen = item.isOpen ? item.isOpen() : false;
        if (!alreadyOpen) {
            if (item.action) item.action();
        }
    } else {
        if (item.action) item.action();
    }
    AddToHistory(item);
    g_open = false;
}

static bool ItemMatchesQuery(const SpotlightItem& item, const std::string& query) {
    std::string hay = item.label + " " + item.category;
    std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
    if (hay.find(query) != std::string::npos) return true;
    for (const auto& kw : item.keywords) {
        std::string kwLower = kw;
        std::transform(kwLower.begin(), kwLower.end(), kwLower.begin(), ::tolower);
        if (kwLower.find(query) != std::string::npos)
            return true;
    }
    return false;
}

void RenderSpotlight() {
    if (!g_open) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420, 320), ImGuiCond_Appearing);

    ImGui::Begin("Spotlight", &g_open,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (g_focusSearch) {
        ImGui::SetKeyboardFocusHere();
        g_focusSearch = false;
    }

    ImGui::InputTextWithHint("##search", "Search menus...", g_search, sizeof(g_search),
        ImGuiInputTextFlags_AutoSelectAll);

    std::string query = g_search;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    struct Match {
        int idx;
    };
    std::vector<Match> matches;

    if (query.empty() && !g_history.empty()) {
        for (const auto& h : g_history) {
            for (int i = 0; i < (int)g_items.size(); i++) {
                if (g_items[i].label == h.label && g_items[i].category == h.category) {
                    matches.push_back({i});
                    break;
                }
            }
        }
    } else {
        for (int i = 0; i < (int)g_items.size(); i++) {
            if (ItemMatchesQuery(g_items[i], query))
                matches.push_back({i});
        }
    }

    bool searchChanged = (g_prevSearch != query);
    if (searchChanged) {
        g_selected = 0;
        g_prevSearch = query;
    }
    if (g_selected >= (int)matches.size()) g_selected = (int)matches.size() - 1;
    if (g_selected < 0 && !matches.empty()) g_selected = 0;

    ImGui::Separator();

    bool hasHeader = (query.empty() && !g_history.empty());

    ImGui::BeginChild("ItemsList", ImVec2(0, 0), ImGuiChildFlags_NavFlattened, 0);

    if (matches.empty()) {
        ImGui::TextDisabled("No matching menus");
    } else {
        if (hasHeader) {
            ImGui::TextDisabled("Recent");
            ImGui::Indent();
        }

        for (int i = 0; i < (int)matches.size(); i++) {
            const auto& item = g_items[matches[i].idx];
            bool selected = (i == g_selected);

            bool open = item.isOpen ? item.isOpen() : false;

            std::string label;
            if (open)
                label = "[x] " + item.category + " > " + item.label;
            else
                label = "[ ] " + item.category + " > " + item.label;

            ImGui::PushID(i);
            if (ImGui::Selectable(label.c_str(), selected)) {
                ActivateItem(item);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }

        if (hasHeader)
            ImGui::Unindent();
    }

    ImGui::EndChild();

    if (ImGui::IsWindowFocused()) {
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && g_selected > 0)
            g_selected--;
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && g_selected < (int)matches.size() - 1)
            g_selected++;
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !matches.empty() && g_selected >= 0 && g_selected < (int)matches.size())
            ActivateItem(g_items[matches[g_selected].idx]);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        g_open = false;

    ImGui::End();
}
