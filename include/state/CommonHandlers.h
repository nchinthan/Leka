#pragma once
#include "state/EditorState.h"

namespace ced::handlers {

// Reusable handlers; each mode picks the ones it wants and registers them.
ActionResult insertCharacter(const Action&, EditorContext&);
ActionResult insertNewline(const Action&, EditorContext&);
ActionResult deleteBackward(const Action&, EditorContext&);
ActionResult deleteForward(const Action&, EditorContext&);
ActionResult moveLeft(const Action&, EditorContext&);
ActionResult moveRight(const Action&, EditorContext&);
ActionResult moveUp(const Action&, EditorContext&);
ActionResult moveDown(const Action&, EditorContext&);
ActionResult moveLineStart(const Action&, EditorContext&);
ActionResult moveLineEnd(const Action&, EditorContext&);
ActionResult switchMode(const Action&, EditorContext&);
ActionResult undo(const Action&, EditorContext&);
ActionResult redo(const Action&, EditorContext&);
ActionResult save(const Action&, EditorContext&);
ActionResult scrollViewport(const Action&, EditorContext&);

} // namespace ced::handlers

namespace ced {

// Base for modes that share cursor movement, undo/redo, save and scrolling.
class NavigableState : public HandlerTableState {
public:
    NavigableState();
};

} // namespace ced
