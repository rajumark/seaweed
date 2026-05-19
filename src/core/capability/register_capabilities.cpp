#include "register_capabilities.h"
#include "capability_registry.h"
#include <sstream>
#include <algorithm>

void RegisterAllCapabilities() {
    auto& reg = CapabilityRegistry::Get();

    reg.Register({
        "list_packages",
        "List Packages",
        "Apps",
        "shell pm list packages",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> packages;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (line.rfind("package:", 0) == 0)
                    packages.push_back(line.substr(8));
            }
            std::sort(packages.begin(), packages.end());
            return packages;
        },
        {"packages", "applications", "installed", "apps"},
        true
    });

    reg.Register({
        "list_system_packages",
        "List System Packages",
        "Apps",
        "shell pm list packages -s",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> packages;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (line.rfind("package:", 0) == 0)
                    packages.push_back(line.substr(8));
            }
            std::sort(packages.begin(), packages.end());
            return packages;
        },
        {"system", "packages"},
        true
    });

    reg.Register({
        "list_user_packages",
        "List User Packages",
        "Apps",
        "shell pm list packages -3",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> packages;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (line.rfind("package:", 0) == 0)
                    packages.push_back(line.substr(8));
            }
            std::sort(packages.begin(), packages.end());
            return packages;
        },
        {"user", "third-party", "packages"},
        true
    });

    reg.Register({
        "list_contacts",
        "List Contacts",
        "Data",
        "shell content query --uri content://com.android.contacts/data",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"contacts", "people", "address book"},
        true
    });

    reg.Register({
        "list_messages",
        "List Messages",
        "Data",
        "shell content query --uri content://sms",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"messages", "sms", "text"},
        true
    });

    reg.Register({
        "list_call_logs",
        "List Call Logs",
        "Data",
        "shell content query --uri content://call_log/calls",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"call logs", "calls", "phone", "history"},
        true
    });

    reg.Register({
        "list_calendar_events",
        "List Calendar Events",
        "Data",
        "shell content query --uri content://com.android.calendar/events",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"calendar", "events", "schedule"},
        true
    });

    reg.Register({
        "list_media",
        "List Media",
        "Data",
        "shell content query --uri content://media/external/images/media",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"media", "images", "photos"},
        true
    });

    reg.Register({
        "list_processes",
        "List Processes",
        "Data",
        "shell dumpsys activity services",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"processes", "services", "running"},
        true
    });

    reg.Register({
        "list_lifecycle",
        "List Lifecycle",
        "Data",
        "shell dumpsys usagestats",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"lifecycle", "usage", "stats"},
        true
    });

    reg.Register({
        "list_properties_system",
        "List System Properties",
        "Data",
        "shell settings list system",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"system", "settings", "properties"},
        true
    });

    reg.Register({
        "list_properties_secure",
        "List Secure Properties",
        "Data",
        "shell settings list secure",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"secure", "settings", "properties"},
        true
    });

    reg.Register({
        "list_properties_global",
        "List Global Properties",
        "Data",
        "shell settings list global",
        CachePolicy::Fixed,
        30000,
        [](const std::string& output) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                if (!line.empty())
                    lines.push_back(line);
            }
            return lines;
        },
        {"global", "settings", "properties"},
        true
    });
}
