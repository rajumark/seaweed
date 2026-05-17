#include "settings_action_helper.h"
#include "settings_pinned_store.h"

namespace SettingsActionHelper {

bool PinSetting(const std::string& settingId) {
    if (settingId.empty()) return false;
    
    bool success = SettingsPinnedStore::PinSetting(settingId);
    return success;
}

bool UnpinSetting(const std::string& settingId) {
    if (settingId.empty()) return false;
    
    bool success = SettingsPinnedStore::UnpinSetting(settingId);
    return success;
}



}
