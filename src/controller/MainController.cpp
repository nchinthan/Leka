#include "controller/MainController.h"

#include "datasource/LocalFileDataSource.h"
#include "datasource/MemoryDataSource.h"
#include "input/KeyMap.h"
#include "renderer/TextLayout.h"
#include "state/CommandState.h"
#include "state/CursorOps.h"
#include "state/InsertState.h"
#include "state/NormalState.h"

#include <algorithm>

namespace ced {

MainController::MainController(ITerminal& terminal)
    : terminal_(terminal), renderer_(terminal), activeMode_(modes::Normal) {
    states_.add(std::make_unique<NormalState>());
    states_.add(std::make_unique<InsertState>());
    states_.add(std::make_unique<CommandState>());
    CommandRegistry::installDefaults(commands_);
    installDefaultKeyBindings(keys_);
    renderer_.addLayer(std::make_unique<DocumentLayer>());
    renderer_.addLayer(std::make_unique<StatusBarLayer>());

    terminal_.size(view_.terminalWidth, view_.terminalHeight);
    // An unopened document behaves as empty until a source is opened.
    std::string ignored;
    openSource(std::make_unique<MemoryDataSource>(std::string{}, "[No Name]", false), ignored);
}

EditorContext MainController::context() {
    return EditorContext{backend_, view_, states_, commands_, commandLine_};
}

bool MainController::openFile(const std::string& path, std::string& error) {
    return openSource(std::make_unique<LocalFileDataSource>(path), error);
}

bool MainController::openSource(std::unique_ptr<IDataSource> source, std::string& error) {
    const bool existed = source->exists();
    const bool ok = backend_.open(std::move(source), error);
    view_ = ViewState{};
    terminal_.size(view_.terminalWidth, view_.terminalHeight);
    status_.message = !ok ? error : (existed ? "" : "New file");
    renderer_.invalidateAll();
    return ok;
}

void MainController::refreshStatus() {
    const EditorState* st = states_.find(activeMode_);
    status_.modeName = st ? st->displayName() : activeMode_;
    status_.fileName = backend_.sourceName();
    status_.modified = backend_.isModified();
    status_.commandActive = activeMode_ == modes::Command;
    status_.commandText = commandLine_.text;
}

bool MainController::transitionTo(const ModeId& mode) {
    EditorState* next = states_.find(mode);
    if (!next) return false;
    EditorContext ctx = context();
    if (EditorState* cur = states_.find(activeMode_)) cur->onExit(ctx);
    activeMode_ = mode;
    next->onEnter(ctx);
    return true;
}

ActionResult MainController::dispatch(const Action& action) {
    EditorState* state = states_.find(activeMode_);
    if (!state) return ActionResult::unhandled();
    EditorContext ctx = context();
    ActionResult result = state->handleAction(action, ctx);
    if (result.nextMode && !transitionTo(*result.nextMode)) result.message = "Unknown mode";
    if (result.quit) quit_ = true;
    EditorContext after = context();
    cursor::clamp(after); // the controller guarantees a valid cursor after every action
    return result;
}

bool MainController::updateViewport() {
    bool changed = false;
    const std::size_t rows = view_.textRows();
    if (view_.cursorLine < view_.firstVisibleLine) {
        view_.firstVisibleLine = view_.cursorLine;
        changed = true;
    } else if (view_.cursorLine >= view_.firstVisibleLine + rows) {
        view_.firstVisibleLine = view_.cursorLine - rows + 1;
        changed = true;
    }
    const std::size_t width = static_cast<std::size_t>(std::max(view_.terminalWidth, 1));
    const std::size_t dcol = displayColumn(backend_.lineText(view_.cursorLine), view_.cursorCol);
    if (dcol < view_.horizontalOffset) {
        view_.horizontalOffset = dcol;
        changed = true;
    } else if (dcol >= view_.horizontalOffset + width) {
        view_.horizontalOffset = dcol - width + 1;
        changed = true;
    }
    return changed;
}

void MainController::draw(DirtyRegion region) {
    if (updateViewport()) region.merge(DirtyRegion::fullViewport());
    refreshStatus();

    int row, col;
    if (activeMode_ == modes::Command) {
        row = view_.terminalHeight - 1;
        col = std::min<int>(1 + static_cast<int>(commandLine_.text.size()), view_.terminalWidth - 1);
    } else {
        const std::size_t dcol = displayColumn(backend_.lineText(view_.cursorLine), view_.cursorCol);
        row = static_cast<int>(view_.cursorLine - view_.firstVisibleLine);
        col = static_cast<int>(dcol - view_.horizontalOffset);
    }
    const RenderContext ctx{backend_, view_, status_, view_.terminalWidth, view_.terminalHeight, row, col};
    renderer_.render(ctx, std::move(region));
}

void MainController::renderNow() { draw(DirtyRegion::fullScreen()); }

bool MainController::processEvent(const InputEvent& event) {
    switch (event.type) {
    case InputEvent::Type::Closed:
        quit_ = true;
        return false;
    case InputEvent::Type::Resize:
        view_.terminalWidth = std::max(event.width, 1);
        view_.terminalHeight = std::max(event.height, 2);
        draw(DirtyRegion::fullScreen());
        return !quit_;
    case InputEvent::Type::Key:
        break;
    }

    status_.message.clear();
    DirtyRegion region;
    for (const Action& action : keys_.resolve(activeMode_, event.key)) {
        const ActionResult r = dispatch(action);
        region.merge(r.dirty);
        if (!r.message.empty()) status_.message = r.message;
        if (quit_) break;
    }
    if (quit_) return false;
    draw(std::move(region));
    return true;
}

void MainController::run() {
    renderNow();
    while (processEvent(terminal_.readEvent())) {
    }
}

} // namespace ced
