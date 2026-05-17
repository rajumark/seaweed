#pragma once
#include <string>
#include "global_config.h"

// Menu state management
enum class MenuScreen {
    APPS_LIST,
    INSTALL_APP,
    CALL_LOGS,
    CONTACTS,
    MESSAGES,
    NOTIFICATIONS,
    LIFECYCLE,
    CALENDAR,
    MEDIA_FILES,
    PROCESSES,
    WIDGETS_LIST,
    KEYBOARD_LIST,
    SETTINGS_SCREEN,
    ADB_TERMINAL
};

void ShowHome();
void SetCurrentScreen(MenuScreen screen);
MenuScreen GetCurrentScreen();
