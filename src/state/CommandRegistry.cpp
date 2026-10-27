#include "state/CommandRegistry.h"

#include "state/CommonHandlers.h"

#include <sstream>

namespace ced {

ActionResult CommandRegistry::execute(const std::string& line, EditorContext& context) const {
    std::istringstream in(line);
    std::string name;
    in >> name;
    if (name.empty()) return ActionResult::ok();

    CommandInvocation inv;
    if (name.back() == '!') {
        inv.force = true;
        name.pop_back();
    }
    for (std::string arg; in >> arg;) inv.args.push_back(arg);

    const auto it = commands_.find(name);
    if (it == commands_.end()) return ActionResult::withMessage("Unknown command: " + name);
    return it->second(inv, context);
}

void CommandRegistry::installDefaults(CommandRegistry& registry) {
    registry.add("w", [](const CommandInvocation&, EditorContext& ctx) {
        return handlers::save(Action::simple(actions::SaveDocument), ctx);
    });
    registry.add("q", [](const CommandInvocation& inv, EditorContext& ctx) {
        if (ctx.document.isModified() && !inv.force)
            return ActionResult::withMessage("Unsaved changes (use :q! to discard)");
        ActionResult r = ActionResult::ok();
        r.quit = true;
        return r;
    });
    registry.add("wq", [](const CommandInvocation&, EditorContext& ctx) {
        ActionResult r = handlers::save(Action::simple(actions::SaveDocument), ctx);
        if (ctx.document.isModified()) return r; // save failed: stay open, message explains why
        r.quit = true;
        return r;
    });
    registry.add("x", [](const CommandInvocation&, EditorContext& ctx) {
        ActionResult r = ctx.document.isModified() ? handlers::save(Action::simple(actions::SaveDocument), ctx)
                                                   : ActionResult::ok();
        if (ctx.document.isModified()) return r;
        r.quit = true;
        return r;
    });
}

} // namespace ced
