#include "state/CommandState.h"

#include "state/CommandRegistry.h"
#include "state/CommonHandlers.h"

namespace ced {
CommandState::CommandState() {
    on(actions::InsertCharacter, [](const Action& a, EditorContext& ctx) {
        if (const std::string* t = a.text()) ctx.commandLine.text += *t;
        return ActionResult::ok();
    });
    on(actions::DeleteBackward, [](const Action&, EditorContext& ctx) {
        ActionResult r = ActionResult::ok();
        if (ctx.commandLine.text.empty()) r.nextMode = std::string(modes::Normal);
        else ctx.commandLine.text.pop_back();
        return r;
    });
    on(actions::InsertNewline, [](const Action&, EditorContext& ctx) {
        const std::string line = ctx.commandLine.text;
        ActionResult r = ctx.commands.execute(line, ctx);
        if (!r.nextMode && !r.quit) r.nextMode = std::string(modes::Normal);
        return r;
    });
    on(actions::SwitchMode, handlers::switchMode);
}
} // namespace ced
