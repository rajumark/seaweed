#pragma once
#include <string>
#include "global_config.h"

enum class DevicePage {
    NONE,
    DEVICES_LIST,
    EMULATORS,
    WIRELESS_ADB,
    DEVELOPER_GUIDE
};

void ShowHome();
void SetDevicePage(DevicePage page);
DevicePage GetDevicePage();
