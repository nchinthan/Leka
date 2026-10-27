#pragma once
#include "actions/Action.h"
#include "actions/ActionResult.h"
#include "state/EditorContext.h"

#include <functional>
#include <map>
#include <string>

namespace ced {

// Abstract editor mode. The controller only ever talks to this interface.
class EditorState {
public:
    virtual ~EditorState() = default;

    virtual ActionResult handleAction(const Action& action, EditorContext& context) = 0;
    virtual ModeId modeId() const = 0;
    virtual std::string displayName() const { return modeId(); }
    virtual void onEnter(EditorContext&) {}
    virtual void onExit(EditorContext&) {}
};

using ActionHandler = std::function<ActionResult(const Action&, EditorContext&)>;

// Convenience base: a mode is a table of action-name -> handler.
// Unknown actions are reported as "unhandled" (they fail safely).
class HandlerTableState : public EditorState {
public:
    ActionResult handleAction(const Action& action, EditorContext& context) override {
        const auto it = handlers_.find(action.id);
        if (it == handlers_.end()) return ActionResult::unhandled();
        return it->second(action, context);
    }

protected:
    void on(std::string_view action, ActionHandler handler) { handlers_[std::string(action)] = std::move(handler); }

private:
    std::map<std::string, ActionHandler, std::less<>> handlers_;
};

} // namespace ced
