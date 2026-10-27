#pragma once
#include "document/TextProcessBackend.h"
#include "state/ViewState.h"

#include <string>

namespace ced {

class StateRegistry;
class CommandRegistry;

// Text being typed after ':' (an extension object kept out of ViewState on purpose).
struct CommandLine {
    std::string text;
};

// The bridge between state behaviour and the rest of the editor.
struct EditorContext {
    TextProcessBackend& document;
    ViewState& view;
    StateRegistry& states;
    CommandRegistry& commands;
    CommandLine& commandLine;
};

} // namespace ced
