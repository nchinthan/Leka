#include "state/InsertState.h"

namespace ced {
InsertState::InsertState() {
    using namespace handlers;
    on(actions::InsertCharacter, insertCharacter);
    on(actions::InsertNewline, insertNewline);
    on(actions::DeleteBackward, deleteBackward);
    on(actions::DeleteForward, deleteForward);
}
} // namespace ced
