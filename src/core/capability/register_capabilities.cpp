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
}
