#pragma once
#include <string>
#include <functional>
#include <SDL.h>

struct SpotlightItem {
    std::string label;
    std::string category;
    std::function<void()> action;
    std::function<bool()> isOpen;
};

void RegisterSpotlightItem(const SpotlightItem& item);
bool SpotlightHandleSDLEvent(const SDL_Event& event);
void RenderSpotlight();
