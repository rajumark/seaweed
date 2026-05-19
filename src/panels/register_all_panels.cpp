#include "register_all_panels.h"
#include "about_panel.h"
#include "theme_panel.h"
#include "device_list_panel.h"
#include "emulator_panel.h"
#include "wireless_panel.h"
#include "apps_panel.h"
#include "contacts_panel.h"
#include "call_logs_panel.h"
#include "messages_panel.h"
#include "settings_panel.h"
#include "calendar_panel.h"
#include "media_panel.h"
#include "processes_panel.h"
#include "lifecycle_panel.h"
#include "device_properties_panel.h"
#include "debug_info_panel.h"

void RegisterAllPanels() {
    RegisterAboutPanel();
    RegisterThemePanel();
    RegisterDeviceListPanel();
    RegisterEmulatorPanel();
    RegisterWirelessPanel();
    RegisterAppsPanel();
    RegisterContactsPanel();
    RegisterCallLogsPanel();
    RegisterMessagesPanel();
    RegisterSettingsPanel();
    RegisterCalendarPanel();
    RegisterMediaPanel();
    RegisterProcessesPanel();
    RegisterLifecyclePanel();
    RegisterDevicePropertiesPanel();
    RegisterDebugInfoPanel();
}
