#include "state/CursorOps.h"

#include <algorithm>

namespace ced::cursor {

std::size_t offsetOf(const EditorContext& ctx) {
    return ctx.document.lineStartOffset(ctx.view.cursorLine) + ctx.view.cursorCol;
}

void setFromOffset(EditorContext& ctx, std::size_t offset) {
    const IDocumentView& doc = ctx.document;
    offset = std::min(offset, doc.size());
    const std::size_t line = doc.lineOfOffset(offset);
    ctx.view.cursorLine = line;
    ctx.view.cursorCol = offset - doc.lineStartOffset(line);
    ctx.view.desiredCol = ctx.view.cursorCol;
}

void clamp(EditorContext& ctx) {
    const IDocumentView& doc = ctx.document;
    ctx.view.cursorLine = std::min(ctx.view.cursorLine, doc.lineCount() - 1);
    ctx.view.cursorCol = std::min(ctx.view.cursorCol, doc.lineLength(ctx.view.cursorLine));
}

DirtyRegion dirtyFor(const IDocumentView& doc, const ChangeInfo& c) {
    const std::size_t off = std::min(c.offset, doc.size());
    const std::size_t line = doc.lineOfOffset(off);
    if (c.lineBreaksTouched) return DirtyRegion::dirtyFromLine(line);
    const std::size_t col = off - doc.lineStartOffset(line);
    return DirtyRegion::dirtyRange(line, col, col + std::max(c.inserted, c.removed));
}

} // namespace ced::cursor
