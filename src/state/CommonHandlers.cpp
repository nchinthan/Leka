#include "state/CommonHandlers.h"

#include "state/CursorOps.h"
#include "state/StateRegistry.h"

#include <algorithm>

namespace ced {
namespace handlers {

namespace {
ActionResult moved() {
    ActionResult r = ActionResult::ok();
    r.cursorMoved = true;
    return r;
}

ActionResult edited(EditorContext& ctx, const ChangeInfo& change, std::size_t newCursorOffset) {
    ActionResult r = ActionResult::ok();
    r.documentChanged = true;
    r.cursorMoved = true;
    r.dirty = cursor::dirtyFor(ctx.document, change);
    cursor::setFromOffset(ctx, newCursorOffset);
    return r;
}

ActionResult rejected(const EditorContext& ctx) {
    return ActionResult::withMessage(ctx.document.canEdit() ? "Nothing to do" : "Document is not editable");
}
} // namespace

ActionResult insertCharacter(const Action& a, EditorContext& ctx) {
    const std::string* text = a.text();
    if (!text || text->empty()) return ActionResult::unhandled();
    const std::size_t at = cursor::offsetOf(ctx);
    auto change = ctx.document.insert(at, *text);
    if (!change) return rejected(ctx);
    return edited(ctx, *change, at + text->size());
}

ActionResult insertNewline(const Action&, EditorContext& ctx) {
    const std::size_t at = cursor::offsetOf(ctx);
    auto change = ctx.document.insert(at, "\n");
    if (!change) return rejected(ctx);
    return edited(ctx, *change, at + 1);
}

ActionResult deleteBackward(const Action&, EditorContext& ctx) {
    const std::size_t at = cursor::offsetOf(ctx);
    if (at == 0) return ActionResult::ok();
    auto change = ctx.document.erase(at - 1, 1);
    if (!change) return rejected(ctx);
    return edited(ctx, *change, at - 1);
}

ActionResult deleteForward(const Action&, EditorContext& ctx) {
    const std::size_t at = cursor::offsetOf(ctx);
    if (at >= ctx.document.size()) return ActionResult::ok();
    auto change = ctx.document.erase(at, 1);
    if (!change) return rejected(ctx);
    return edited(ctx, *change, at);
}

ActionResult moveLeft(const Action&, EditorContext& ctx) {
    ViewState& v = ctx.view;
    if (v.cursorCol > 0) {
        --v.cursorCol;
    } else if (v.cursorLine > 0) {
        --v.cursorLine;
        v.cursorCol = ctx.document.lineLength(v.cursorLine);
    }
    v.desiredCol = v.cursorCol;
    return moved();
}

ActionResult moveRight(const Action&, EditorContext& ctx) {
    ViewState& v = ctx.view;
    if (v.cursorCol < ctx.document.lineLength(v.cursorLine)) {
        ++v.cursorCol;
    } else if (v.cursorLine + 1 < ctx.document.lineCount()) {
        ++v.cursorLine;
        v.cursorCol = 0;
    }
    v.desiredCol = v.cursorCol;
    return moved();
}

ActionResult moveUp(const Action&, EditorContext& ctx) {
    ViewState& v = ctx.view;
    if (v.cursorLine > 0) {
        --v.cursorLine;
        v.cursorCol = std::min(v.desiredCol, ctx.document.lineLength(v.cursorLine));
    }
    return moved();
}

ActionResult moveDown(const Action&, EditorContext& ctx) {
    ViewState& v = ctx.view;
    if (v.cursorLine + 1 < ctx.document.lineCount()) {
        ++v.cursorLine;
        v.cursorCol = std::min(v.desiredCol, ctx.document.lineLength(v.cursorLine));
    }
    return moved();
}

ActionResult moveLineStart(const Action&, EditorContext& ctx) {
    ctx.view.cursorCol = ctx.view.desiredCol = 0;
    return moved();
}

ActionResult moveLineEnd(const Action&, EditorContext& ctx) {
    ctx.view.cursorCol = ctx.view.desiredCol = ctx.document.lineLength(ctx.view.cursorLine);
    return moved();
}

ActionResult switchMode(const Action& a, EditorContext& ctx) {
    const std::string* mode = a.text();
    if (!mode || !ctx.states.contains(*mode)) return ActionResult::withMessage("Unknown mode");
    ActionResult r = ActionResult::ok();
    r.nextMode = *mode;
    return r;
}

ActionResult undo(const Action&, EditorContext& ctx) {
    auto out = ctx.document.undo();
    if (!out) return ActionResult::withMessage("Nothing to undo");
    return edited(ctx, out->change, out->cursorOffset);
}

ActionResult redo(const Action&, EditorContext& ctx) {
    auto out = ctx.document.redo();
    if (!out) return ActionResult::withMessage("Nothing to redo");
    return edited(ctx, out->change, out->cursorOffset);
}

ActionResult save(const Action&, EditorContext& ctx) {
    std::string err;
    if (!ctx.document.save(err)) return ActionResult::withMessage("Save failed: " + err);
    return ActionResult::withMessage("Saved " + ctx.document.sourceName());
}

ActionResult scrollViewport(const Action& a, EditorContext& ctx) {
    ViewState& v = ctx.view;
    const long long maxFirst = static_cast<long long>(ctx.document.lineCount()) - 1;
    const long long next = std::clamp(static_cast<long long>(v.firstVisibleLine) + a.number(), 0LL, maxFirst);
    v.firstVisibleLine = static_cast<std::size_t>(next);
    // Keep the cursor inside the viewport (otherwise the controller would scroll straight back).
    const std::size_t lastLine = std::min(v.lastVisibleLine(), static_cast<std::size_t>(maxFirst));
    v.cursorLine = std::clamp(v.cursorLine, v.firstVisibleLine, lastLine);
    v.cursorCol = std::min(v.desiredCol, ctx.document.lineLength(v.cursorLine));
    ActionResult r = moved();
    r.dirty = DirtyRegion::fullViewport();
    return r;
}

} // namespace handlers

NavigableState::NavigableState() {
    using namespace handlers;
    on(actions::MoveCursorLeft, moveLeft);
    on(actions::MoveCursorRight, moveRight);
    on(actions::MoveCursorUp, moveUp);
    on(actions::MoveCursorDown, moveDown);
    on(actions::MoveLineStart, moveLineStart);
    on(actions::MoveLineEnd, moveLineEnd);
    on(actions::SwitchMode, switchMode);
    on(actions::Undo, undo);
    on(actions::Redo, redo);
    on(actions::SaveDocument, save);
    on(actions::ScrollViewport, scrollViewport);
}

} // namespace ced
