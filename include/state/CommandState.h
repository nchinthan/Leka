#pragma once
#include "state/EditorState.h"

namespace ced {
class CommandState final : public HandlerTableState {
public:
    CommandState();
    ModeId modeId() const override { return std::string(modes::Command); }
    std::string displayName() const override { return "COMMAND"; }
    void onEnter(EditorContext& ctx) override { ctx.commandLine.text.clear(); }
    void onExit(EditorContext& ctx) override { ctx.commandLine.text.clear(); }
};
} // namespace ced
