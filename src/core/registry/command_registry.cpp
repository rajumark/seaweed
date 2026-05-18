#include "command_registry.h"
#include <algorithm>

CommandRegistry& CommandRegistry::Get() {
    static CommandRegistry instance;
    return instance;
}

void CommandRegistry::Register(const Command& cmd) {
    m_commands.push_back(cmd);
}

const std::vector<Command>& CommandRegistry::GetAll() const {
    return m_commands;
}

std::vector<const Command*> CommandRegistry::Search(const std::string& query) const {
    std::vector<const Command*> results;
    if (query.empty()) {
        results.reserve(m_commands.size());
        for (const auto& cmd : m_commands)
            results.push_back(&cmd);
        return results;
    }

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    for (const auto& cmd : m_commands) {
        std::string lowerLabel = cmd.label;
        std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::tolower);
        if (lowerLabel.find(lowerQuery) != std::string::npos) {
            results.push_back(&cmd);
            continue;
        }
        std::string lowerCategory = cmd.category;
        std::transform(lowerCategory.begin(), lowerCategory.end(), lowerCategory.begin(), ::tolower);
        if (lowerCategory.find(lowerQuery) != std::string::npos) {
            results.push_back(&cmd);
            continue;
        }
        for (const auto& kw : cmd.keywords) {
            std::string lowerKw = kw;
            std::transform(lowerKw.begin(), lowerKw.end(), lowerKw.begin(), ::tolower);
            if (lowerKw.find(lowerQuery) != std::string::npos) {
                results.push_back(&cmd);
                break;
            }
        }
    }
    return results;
}
