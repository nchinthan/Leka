#pragma once
#include "actions/Action.h"
#include "renderer/DirtyRegion.h"

#include <optional>
#include <string>

namespace ced {

// Coordination info returned by a state after handling an action.
struct ActionResult {
    bool handled = false;
    bool documentChanged = false;
    bool cursorMoved = false;
    bool quit = false;
    std::optional<ModeId> nextMode; // set => controller performs the mode transition
    DirtyRegion dirty;
    std::string message;            // shown in the status bar

    static ActionResult unhandled() { return {}; }
    static ActionResult ok() { ActionResult r; r.handled = true; return r; }
    static ActionResult withMessage(std::string m) { ActionResult r = ok(); r.message = std::move(m); return r; }
};

} // namespace ced
