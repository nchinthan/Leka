#include "input/KeyMap.h"

namespace ced {

void installDefaultKeyBindings(KeyBindingRegistry& r) {
    using namespace actions;
    const auto G = modes::Global;

    // Global (every mode)
    r.bind(G, KeyChord::special(Key::Left), Action::simple(MoveCursorLeft));
    r.bind(G, KeyChord::special(Key::Right), Action::simple(MoveCursorRight));
    r.bind(G, KeyChord::special(Key::Up), Action::simple(MoveCursorUp));
    r.bind(G, KeyChord::special(Key::Down), Action::simple(MoveCursorDown));
    r.bind(G, KeyChord::special(Key::Home), Action::simple(MoveLineStart));
    r.bind(G, KeyChord::special(Key::End), Action::simple(MoveLineEnd));
    r.bind(G, KeyChord::special(Key::Enter), Action::simple(InsertNewline));
    r.bind(G, KeyChord::special(Key::Backspace), Action::simple(DeleteBackward));
    r.bind(G, KeyChord::special(Key::Delete), Action::simple(DeleteForward));
    r.bind(G, KeyChord::special(Key::Escape), Action::switchMode(modes::Normal));
    r.bind(G, KeyChord::ctrlChr('s'), Action::simple(SaveDocument));
    r.bind(G, KeyChord::ctrlChr('z'), Action::simple(Undo));
    r.bind(G, KeyChord::ctrlChr('y'), Action::simple(Redo));
    r.bind(G, KeyChord::special(Key::PageUp), Action::withNumber(ScrollViewport, -20));
    r.bind(G, KeyChord::special(Key::PageDown), Action::withNumber(ScrollViewport, 20));

    // Insert mode: every printable character is text; Tab inserts a tab.
    r.bindFallback(modes::Insert, KeyBindingRegistry::isPrintable, [](const KeyEvent& e) {
        return std::vector<Action>{Action::withText(InsertCharacter, std::string(1, e.ch))};
    });
    r.bind(modes::Insert, KeyChord::special(Key::Tab), Action::withText(InsertCharacter, "\t"));

    // Command mode: printable characters build the command line.
    r.bindFallback(modes::Command, KeyBindingRegistry::isPrintable, [](const KeyEvent& e) {
        return std::vector<Action>{Action::withText(InsertCharacter, std::string(1, e.ch))};
    });

    // Normal mode: vi-style keys. 'a', 'A', 'I', 'o' are action *sequences*.
    const auto N = modes::Normal;
    const Action toInsert = Action::switchMode(modes::Insert);
    r.bind(N, KeyChord::chr('h'), Action::simple(MoveCursorLeft));
    r.bind(N, KeyChord::chr('j'), Action::simple(MoveCursorDown));
    r.bind(N, KeyChord::chr('k'), Action::simple(MoveCursorUp));
    r.bind(N, KeyChord::chr('l'), Action::simple(MoveCursorRight));
    r.bind(N, KeyChord::chr('0'), Action::simple(MoveLineStart));
    r.bind(N, KeyChord::chr('$'), Action::simple(MoveLineEnd));
    r.bind(N, KeyChord::chr('x'), Action::simple(DeleteForward));
    r.bind(N, KeyChord::chr('u'), Action::simple(Undo));
    r.bind(N, KeyChord::ctrlChr('r'), Action::simple(Redo));
    r.bind(N, KeyChord::chr(':'), Action::switchMode(modes::Command));
    r.bind(N, KeyChord::chr('i'), toInsert);
    r.bind(N, KeyChord::chr('a'), std::vector<Action>{Action::simple(MoveCursorRight), toInsert});
    r.bind(N, KeyChord::chr('A'), std::vector<Action>{Action::simple(MoveLineEnd), toInsert});
    r.bind(N, KeyChord::chr('I'), std::vector<Action>{Action::simple(MoveLineStart), toInsert});
    r.bind(N, KeyChord::chr('o'),
           std::vector<Action>{Action::simple(MoveLineEnd), toInsert, Action::simple(InsertNewline)});
}

} // namespace ced
