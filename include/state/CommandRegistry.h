#pragma once
#include "actions/ActionResult.h"
#include "state/EditorContext.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ced {

struct CommandInvocation {
    std::vector<std::string> args;
    bool force = false; // name ended with '!'
};

using CommandHandler = std::function<ActionResult(const CommandInvocation&, EditorContext&)>;

// ":" commands (w, q, wq, ...). New commands are registered, never hard-coded in the state.
class CommandRegistry {
public:
    void add(std::string name, CommandHandler handler) { commands_[std::move(name)] = std::move(handler); }
    ActionResult execute(const std::string& line, EditorContext& context) const;
    static void installDefaults(CommandRegistry& registry);

private:
    std::map<std::string, CommandHandler, std::less<>> commands_;
};

} // namespace ced
