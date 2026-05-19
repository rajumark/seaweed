#include "apps_panel.h"
#include "core/registry/panel_registry.h"
#include "apps_action_cook_helper.h"
#include "global_config.h"
#include "device_manager.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <thread>
#include <mutex>
#include <atomic>

enum class AppFilter {
    All, User, System, Enabled, Disabled, Uninstalled
};

static const char* AppFilterLabel(AppFilter f) {
    switch (f) {
        case AppFilter::All:        return "All Apps";
        case AppFilter::User:       return "User Apps";
        case AppFilter::System:     return "System Apps";
        case AppFilter::Enabled:    return "Enabled Apps";
        case AppFilter::Disabled:   return "Disabled Apps";
        case AppFilter::Uninstalled:return "Uninstalled Apps";
    }
    return "";
}

static const char* AppFilterFlag(AppFilter f) {
    switch (f) {
        case AppFilter::All:        return "";
        case AppFilter::User:       return " -3";
        case AppFilter::System:     return " -s";
        case AppFilter::Enabled:    return " -e";
        case AppFilter::Disabled:   return " -d";
        case AppFilter::Uninstalled:return " -u";
    }
    return "";
}

static AppFilter& operator++(AppFilter& f) {
    f = static_cast<AppFilter>(static_cast<int>(f) + 1);
    return f;
}

static AppFilter operator++(AppFilter& f, int) {
    AppFilter r = f;
    ++f;
    return r;
}

static AppFilter& operator--(AppFilter& f) {
    f = static_cast<AppFilter>(static_cast<int>(f) - 1);
    return f;
}

static AppFilter operator--(AppFilter& f, int) {
    AppFilter r = f;
    --f;
    return r;
}

static std::string TrimStr(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
}

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::string ExecAdb(const std::string& deviceId, const std::string& args) {
    std::string cmd = GlobalConfig::BuildAdbCommand("-s " + deviceId + " " + args);
    return DeviceManager::GetInstance().ExecuteCommand(cmd);
}

struct BasicInfoRow {
    std::string key;
    std::string value;
    std::string description;
};

struct PermissionItem {
    std::string name;
    bool granted = false;
};

static std::string DescribeKey(const std::string& key) {
    static const std::map<std::string, std::string> descs = {
        {"appId", "Application ID (same as package name)"},
        {"pkg", "Package name of the app"},
        {"versionName", "Human-readable version name"},
        {"versionCode", "Internal version code used by the system"},
        {"minSdk", "Minimum Android SDK level required"},
        {"targetSdk", "Target Android SDK level"},
        {"installerPackageName", "Package responsible for installing"},
        {"installerPackageUid", "UID of the installer package"},
        {"initiatingPackageName", "Package that initiated installation"},
        {"originatingPackageName", "Original source package for install"},
        {"updateOwnerPackageName", "Package that owns updates"},
        {"packageSource", "Source of the package"},
        {"timeStamp", "Initial install timestamp"},
        {"lastUpdateTime", "Last update timestamp"},
        {"codePath", "Path to base APK on device"},
        {"resourcePath", "Path to resources"},
        {"legacyNativeLibraryDir", "Directory for native libraries"},
        {"extractNativeLibs", "Whether native libs are extracted"},
        {"primaryCpuAbi", "Primary supported CPU ABI"},
        {"usesNonSdkApi", "Whether app uses non-SDK APIs"},
        {"splits", "APK splits present"},
        {"apkSigningVersion", "APK signing scheme version"},
        {"flags", "Package flags (bitmask)"},
        {"privateFlags", "Private package flags"},
        {"forceQueryable", "Whether app is force-queryable"},
        {"queriesPackages", "Packages this app can query"},
        {"queriesIntents", "Intents this app can query"},
        {"dataDir", "App data directory on device"},
        {"supportsScreens", "Screen sizes supported"},
        {"appMetadataFilePath", "Path to app metadata file"},
        {"installPermissionsFixed", "Install permissions fixed"},
        {"isMiuiPreinstall", "MIUI preinstalled flag"},
    };
    auto it = descs.find(key);
    return it != descs.end() ? it->second : "";
}

static std::vector<BasicInfoRow> ParseBasicInfo(const std::string& dump) {
    std::vector<BasicInfoRow> rows;
    std::istringstream iss(dump);
    std::string line;
    while (std::getline(iss, line)) {
        line = TrimStr(line);
        auto eq = line.find('=');
        if (eq == std::string::npos || eq == 0) continue;
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        rows.push_back({key, value, DescribeKey(key)});
    }
    return rows;
}

static std::map<std::string, std::vector<PermissionItem>> ParsePermissions(const std::string& dump) {
    std::map<std::string, std::vector<PermissionItem>> result;
    std::istringstream iss(dump);
    std::string line;
    std::string currentGroup;
    while (std::getline(iss, line)) {
        if (line.find("requested permissions:") != std::string::npos) {
            currentGroup = "requested_permissions";
            continue;
        }
        if (line.find("install permissions:") != std::string::npos) {
            currentGroup = "install_permissions";
            continue;
        }
        if (line.find("runtime permissions:") != std::string::npos) {
            currentGroup = "runtime_permissions";
            continue;
        }
        if (line.empty() && !currentGroup.empty()) {
            currentGroup.clear();
            continue;
        }
        if (currentGroup.empty()) continue;
        auto pos = line.find("android.permission.");
        if (pos == std::string::npos) continue;
        std::string perm;
        for (size_t i = pos; i < line.size(); i++) {
            if (line[i] == ':' || line[i] == ' ') break;
            perm += line[i];
        }
        if (perm.empty()) continue;
        bool granted = line.find("granted=true") != std::string::npos;
        result[currentGroup].push_back({perm, granted});
    }
    return result;
}

static std::vector<std::string> ParsePaths(const std::string& output) {
    std::vector<std::string> paths;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        line = TrimStr(line);
        if (line.empty()) continue;
        auto pos = line.find("package:");
        if (pos != std::string::npos)
            line = line.substr(pos + 8);
        paths.push_back(line);
    }
    return paths;
}

static std::map<std::string, std::string> ParseSections(const std::string& dump) {
    std::map<std::string, std::string> sections;
    std::istringstream iss(dump);
    std::string line;
    std::string currentTitle;
    std::string currentContent;
    while (std::getline(iss, line)) {
        bool isHeader = !line.empty() && line.back() == ':'
                     && (line[0] != ' ' && line[0] != '\t');
        if (isHeader) {
            if (!currentTitle.empty()) {
                sections[currentTitle] = currentContent;
            }
            currentTitle = TrimStr(line.substr(0, line.size() - 1));
            currentContent.clear();
        } else {
            if (!currentContent.empty()) currentContent += "\n";
            currentContent += line;
        }
    }
    if (!currentTitle.empty())
        sections[currentTitle] = currentContent;
    return sections;
}

class AppsPanel {
public:
    AppsPanel()
        : m_title("Apps")
        , m_refreshMs(5000)
        , m_currentFilter(AppFilter::User)
    {}

    void Draw() {
        static bool mockInitialized = false;
        if (!mockInitialized) {
            mockInitialized = true;
            m_show = true;
            m_selectedPkg = "com.raju.shingadiya.debug";
            m_showDetailsPanel = true;
            m_detailPhase = DetailPhase::WithData;
            m_basicInfoRows.push_back({ "pkg", "com.raju.shingadiya.debug", "Package name" });
            m_basicInfoRows.push_back({ "versionName", "1.0.0", "Version name" });
            m_permissions["requested_permissions"].push_back({ "android.permission.INTERNET", true });
            m_sections["Header"] = "Content";
            m_pathList.push_back("/data/app/com.raju.shingadiya.debug/base.apk");
        }
        if (!m_show) return;
        if (!ImGui::Begin(m_title.c_str(), &m_show)) {
            ImGui::End();
            return;
        }

        auto now = std::chrono::steady_clock::now();

        std::string currentDevice = GlobalConfig::GetSelectedDeviceId();
        static bool firstTime = true;
        if (firstTime && !currentDevice.empty()) {
            firstTime = false;
            m_selectedPkg = "com.raju.shingadiya.debug";
            m_showDetailsPanel = true;
            m_detailTab = 0;
            StartDetailFetch(m_selectedPkg, currentDevice);
        }
        if (currentDevice != m_cachedDeviceId) {
            m_cachedDeviceId = currentDevice;
            m_phase = Phase::Idle;
            m_packages.clear();
        }

        if (m_phase == Phase::Loading) {
            // Loading state handled by background thread
        }

        if (m_phase == Phase::Idle && !m_cachedDeviceId.empty()) {
            TriggerRefresh();
        }

        if (m_cachedDeviceId.empty()) {
            ImGui::TextDisabled("No device selected. Connect a device first.");
        } else if (m_phase == Phase::Loading && m_packages.empty()) {
            ImGui::TextDisabled("Loading packages...");
        } else {
            RenderContent();
        }

        ImGui::End();

        RenderDetailWindow();
        CheckDetailThread();
    }

    bool* GetShowPtr() { return &m_show; }

private:
    enum class Phase { Idle, Loading, WithData };
    enum class DetailPhase { Idle, Loading, WithData };

    void RenderContent() {
        RenderToolbar();

        ImGui::Separator();

        ImGui::BeginChild("PackageList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        std::string filterLower = ToLower(m_filter);
        std::string currentDevice = m_cachedDeviceId;

        for (size_t i = 0; i < m_packages.size(); i++) {
            const auto& pkg = m_packages[i];

            if (!m_filter.empty()) {
                std::string pkgLower = ToLower(pkg);
                if (pkgLower.find(filterLower) == std::string::npos)
                    continue;
            }

            bool selected = (m_selectedPkg == pkg);
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::Selectable(pkg.c_str(), selected)) {
                m_selectedPkg = pkg;
                m_showDetailsPanel = true;
                m_detailTab = 0;
                StartDetailFetch(pkg, currentDevice);
            }
            if (ImGui::BeginPopupContextItem())
                HandleContextMenu(pkg);
            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void RenderToolbar() {
        ImGui::Text("Total: %zu", m_packages.size());
        ImGui::SameLine();

        // Filter dropdown
        static const char* filterLabels[] = {
            "All Apps", "User Apps", "System Apps", "Enabled Apps", "Disabled Apps", "Uninstalled Apps"
        };
        static int filterCurrent = 1; // User Apps = index 1
        if (ImGui::BeginCombo("##filter_picker", filterLabels[filterCurrent])) {
            for (int n = 0; n < IM_ARRAYSIZE(filterLabels); n++) {
                bool is_selected = (filterCurrent == n);
                if (ImGui::Selectable(filterLabels[n], is_selected)) {
                    filterCurrent = n;
                    m_currentFilter = static_cast<AppFilter>(n);
                    TriggerRefresh();
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();

        if (ImGui::SmallButton("Refresh")) {
            TriggerRefresh();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::InputTextWithHint("##filter", "Search packages...", m_filterBuf, sizeof(m_filterBuf))) {
            m_filter = m_filterBuf;
        }
    }

    void StartDetailFetch(const std::string& pkg, const std::string& deviceId) {
        m_detailPhase = DetailPhase::Loading;
        m_detailDump.clear();
        m_detailPaths.clear();
        m_basicInfoRows.clear();
        m_permissions.clear();
        m_sections.clear();
        m_pathList.clear();

        int reqId = ++m_detailRequestId;
        std::thread([this, reqId, pkg, deviceId]() {
            std::string dump = ExecAdb(deviceId, "shell dumpsys package " + pkg);
            std::string paths = ExecAdb(deviceId, "shell pm path " + pkg);
            {
                std::lock_guard<std::mutex> lock(m_detailMutex);
                if (reqId != m_detailRequestId.load()) return;
                m_detailDump = dump;
                m_detailPaths = paths;
                m_detailDone = true;
            }
        }).detach();
    }

    void CheckDetailThread() {
        if (m_detailPhase != DetailPhase::Loading) return;
        std::lock_guard<std::mutex> lock(m_detailMutex);
        if (!m_detailDone) return;
        m_detailDone = false;
        m_basicInfoRows = ParseBasicInfo(m_detailDump);
        m_permissions = ParsePermissions(m_detailDump);
        m_sections = ParseSections(m_detailDump);
        m_pathList = ParsePaths(m_detailPaths);
        m_detailPhase = DetailPhase::WithData;
    }

    void RenderDetailWindow() {
        if (!m_showDetailsPanel || m_selectedPkg.empty()) return;
        ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
        std::string winTitle = "App Details: " + m_selectedPkg;
        if (!ImGui::Begin(winTitle.c_str(), &m_showDetailsPanel)) {
            ImGui::End();
            return;
        }

        if (m_detailPhase == DetailPhase::Loading) {
            ImGui::TextDisabled("Loading details for %s...", m_selectedPkg.c_str());
            ImGui::End();
            return;
        }

        if (ImGui::BeginTabBar("AppDetailTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Basic Info")) {
                RenderBasicInfoTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Permissions")) {
                RenderPermissionsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Paths")) {
                RenderPathsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Full Info")) {
                RenderFullInfoTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void RenderBasicInfoTab() {
        if (m_basicInfoRows.empty()) {
            ImGui::TextDisabled("No basic info available.");
            return;
        }

        ImGui::Text("Package: %s", m_selectedPkg.c_str());
        ImGui::Separator();

        static char infoFilterBuf[256] = {};
        ImGui::SetNextItemWidth(300);
        ImGui::InputTextWithHint("##info-filter", "Search keys/values...", infoFilterBuf, sizeof(infoFilterBuf));
        std::string infoFilter = ToLower(infoFilterBuf);
        ImGui::Separator();

        if (ImGui::BeginTable("BasicInfo", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY,
            ImVec2(0, -ImGui::GetTextLineHeightWithSpacing())))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Key");
            ImGui::TableSetupColumn("Value");
            ImGui::TableSetupColumn("Description");
            ImGui::TableHeadersRow();

            for (const auto& row : m_basicInfoRows) {
                if (!infoFilter.empty()) {
                    bool k = ToLower(row.key).find(infoFilter) != std::string::npos;
                    bool v = ToLower(row.value).find(infoFilter) != std::string::npos;
                    bool d = ToLower(row.description).find(infoFilter) != std::string::npos;
                    if (!k && !v && !d) continue;
                }
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", row.key.c_str());
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", row.value.c_str());
                ImGui::TableNextColumn();
                ImGui::TextDisabled("%s", row.description.c_str());
            }
            ImGui::EndTable();
        }
    }

    void RenderPermissionsTab() {
        if (m_permissions.empty()) {
            ImGui::TextDisabled("No permissions data available.");
            return;
        }

        static const char* groupLabels[] = {
            "requested_permissions", "install_permissions", "runtime_permissions"
        };
        static const char* groupNames[] = {
            "Requested Permissions", "Install Permissions", "Runtime Permissions"
        };

        static int selectedGroup = 2;
        for (int i = 0; i < 3; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::RadioButton(groupNames[i], &selectedGroup, i))
                ;
        }

        ImGui::Separator();

        std::string groupKey = groupLabels[selectedGroup];
        auto it = m_permissions.find(groupKey);
        if (it == m_permissions.end()) {
            ImGui::TextDisabled("No %s.", groupNames[selectedGroup]);
            return;
        }

        static char permFilterBuf[256] = {};
        ImGui::SetNextItemWidth(300);
        ImGui::InputTextWithHint("##perm-filter", "Search permissions...", permFilterBuf, sizeof(permFilterBuf));
        std::string permFilter = ToLower(permFilterBuf);
        ImGui::Separator();

        ImGui::Text("Total: %zu", it->second.size());
        ImGui::Separator();

        ImGui::BeginChild("PermissionsList", ImVec2(0, 0), true);
        for (const auto& perm : it->second) {
            if (!permFilter.empty() && ToLower(perm.name).find(permFilter) == std::string::npos)
                continue;

            if (selectedGroup == 2) {
                bool checked = perm.granted;
                ImGui::Checkbox(perm.name.c_str(), &checked);
            } else {
                ImGui::Text("%s%s",
                    perm.name.c_str(),
                    perm.granted ? " (granted)" : "");
            }
        }
        ImGui::EndChild();
    }

    void RenderPathsTab() {
        if (m_pathList.empty()) {
            ImGui::TextDisabled("No paths available.");
            return;
        }

        for (const auto& path : m_pathList) {
            if (ImGui::Selectable(path.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    std::string deviceId = m_cachedDeviceId;
                    std::thread([path, deviceId]() {
                        ExecAdb(deviceId, "pull " + path + " ~/Downloads/");
                    }).detach();
                }
            }
            if (ImGui::BeginItemTooltip()) {
                ImGui::Text("Double-click to pull to ~/Downloads/");
                ImGui::EndTooltip();
            }
        }
    }

    void RenderFullInfoTab() {
        if (m_sections.empty()) {
            ImGui::TextDisabled("No sections available.");
            return;
        }

        std::vector<std::string> sectionKeys;
        for (const auto& [k, v] : m_sections)
            sectionKeys.push_back(k);

        static char sectionFilterBuf[256] = {};
        ImGui::SetNextItemWidth(300);
        ImGui::InputTextWithHint("##section-filter", "Search sections...", sectionFilterBuf, sizeof(sectionFilterBuf));
        std::string sectionFilter = ToLower(sectionFilterBuf);
        ImGui::Separator();

        float leftWidth = 250.0f;
        ImGui::BeginChild("SectionList", ImVec2(leftWidth, 0),             ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
        for (size_t i = 0; i < sectionKeys.size(); i++) {
            if (!sectionFilter.empty() && ToLower(sectionKeys[i]).find(sectionFilter) == std::string::npos)
                continue;
            if (ImGui::Selectable(sectionKeys[i].c_str(), m_selectedSection == static_cast<int>(i)))
                m_selectedSection = static_cast<int>(i);
        }
        ImGui::EndChild();
        ImGui::SameLine();

        if (m_selectedSection >= 0 && m_selectedSection < static_cast<int>(sectionKeys.size())) {
            const auto& content = m_sections[sectionKeys[m_selectedSection]];
            ImGui::BeginChild("SectionContent", ImVec2(0, 0), ImGuiChildFlags_Borders);
            ImGui::TextWrapped("%s", content.c_str());
            ImGui::EndChild();
        } else {
            ImGui::BeginChild("SectionContent", ImVec2(0, 0));
            ImGui::TextDisabled("Select a section on the left.");
            ImGui::EndChild();
        }
    }

    void HandleContextMenu(const std::string& pkg) {
        if (ImGui::MenuItem("Open"))
            AppsActionCookHelper::StartAppAsync(pkg);
        if (ImGui::MenuItem("Force Stop"))
            AppsActionCookHelper::ForceStopAppAsync(pkg);
        if (ImGui::MenuItem("Restart"))
            AppsActionCookHelper::RestartAppAsync(pkg);
        ImGui::Separator();
        if (ImGui::MenuItem("Uninstall"))
            AppsActionCookHelper::UninstallAppAsync(pkg);
        if (ImGui::MenuItem("Clear Data"))
            AppsActionCookHelper::ClearAppDataAsync(pkg);
        ImGui::Separator();
        if (ImGui::MenuItem("Enable"))
            AppsActionCookHelper::EnableAppAsync(pkg);
        if (ImGui::MenuItem("Disable"))
            AppsActionCookHelper::DisableAppAsync(pkg);
        ImGui::Separator();
        if (ImGui::MenuItem("Open App Info"))
            AppsActionCookHelper::OpenAppInfoAsync(pkg);
        if (ImGui::MenuItem("Copy Package Name"))
            AppsActionCookHelper::CopyPackageName(pkg);
        ImGui::EndPopup();
    }

    void TriggerRefresh() {
        m_phase = Phase::Loading;
        m_packages.clear();
        m_lastRefresh = std::chrono::steady_clock::now();
        std::string deviceId = m_cachedDeviceId;
        std::thread([this, deviceId]() {
            std::string args = std::string("shell pm list packages") + AppFilterFlag(m_currentFilter);
            std::string output = ExecAdb(deviceId, args);
            {
                std::lock_guard<std::mutex> lock(m_detailMutex); // Reuse mutex for package list
                m_packages = [&]() {
                    std::vector<std::string> lines;
                    std::istringstream iss(output);
                    std::string line;
                    while (std::getline(iss, line)) {
                        line = TrimStr(line);
                        if (line.rfind("package:", 0) == 0) {
                            lines.push_back(line.substr(8)); // strip "package:" prefix
                        }
                    }
                    return lines;
                }();
            }
        }).detach();
    }

    bool m_show = false;
    bool m_showDetailsPanel = false;
    std::string m_selectedPkg;
    int m_selectedSection = -1;
    int m_detailTab = 0;
    char m_filterBuf[256] = {};
    std::string m_filter;
    std::vector<std::string> m_packages;

    std::string m_title;
    int m_refreshMs;
    Phase m_phase = Phase::Idle;
    std::string m_cachedDeviceId;
    std::chrono::steady_clock::time_point m_lastRefresh;
    AppFilter m_currentFilter;

    DetailPhase m_detailPhase = DetailPhase::Idle;
    std::string m_detailDump;
    std::string m_detailPaths;
    std::atomic<int> m_detailRequestId{0};
    bool m_detailDone = false;
    std::mutex m_detailMutex;

    std::vector<BasicInfoRow> m_basicInfoRows;
    std::map<std::string, std::vector<PermissionItem>> m_permissions;
    std::map<std::string, std::string> m_sections;
    std::vector<std::string> m_pathList;
};

static AppsPanel* s_panel = nullptr;

static void DrawAppsPanel() {
    if (!s_panel) return;
    s_panel->Draw();
}

void RegisterAppsPanel() {
    s_panel = new AppsPanel();
    PanelRegistry::Get().Register({
        "apps",
        "Apps",
        "Data",
        DrawAppsPanel,
        s_panel->GetShowPtr(),
        {"packages", "applications", "installed"}
    });
}
