#include "register_all_panels.h"
#include "about_panel.h"
#include "theme_panel.h"
#include "device_list_panel.h"
#include "emulator_panel.h"
#include "wireless_panel.h"

void RegisterAllPanels() {
    RegisterAboutPanel();
    RegisterThemePanel();
    RegisterDeviceListPanel();
    RegisterEmulatorPanel();
    RegisterWirelessPanel();
}
