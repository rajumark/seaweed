#pragma once
#include <string>
#include <vector>
#include <functional>

struct Command {
    std::string id;
    std::string label;
    std::string category;
    std::vector<std::string> keywords;
    std::function<void()> execute;
    std::function<bool()> isToggled;
};

class CommandRegistry {
public:
    static CommandRegistry& Get();

    void Register(const Command& cmd);

    const std::vector<Command>& GetAll() const;
    std::vector<const Command*> Search(const std::string& query) const;

private:
    CommandRegistry() = default;
    std::vector<Command> m_commands;
};
