#include "state/NormalState.h"

namespace ced {
// Same semantic actions as insert mode, interpreted differently:
// no text insertion; Backspace/Enter navigate; DeleteForward deletes under the cursor ('x').
NormalState::NormalState() {
    using namespace handlers;
    on(actions::DeleteForward, deleteForward);
    on(actions::DeleteBackward, moveLeft);
    on(actions::InsertNewline, moveDown);
}
} // namespace ced
