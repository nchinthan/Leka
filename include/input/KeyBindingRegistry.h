#pragma once
#include "actions/Action.h"
#include "input/InputEvent.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ced {

// Identifies a key combination (what a binding is attached to).
struct KeyChord {
    Key key = Key::Char;
    char ch = 0;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;

    static KeyChord chr(char c) { KeyChord k; k.ch = c; return k; }
    static KeyChord ctrlChr(char c) { KeyChord k; k.ch = c; k.ctrl = true; return k; }
    static KeyChord special(Key key, bool ctrl = false) { KeyChord k; k.key = key; k.ctrl = ctrl; return k; }
    static KeyChord from(const KeyEvent& e) {
        KeyChord k{e.key, e.ch, e.ctrl, e.alt, e.shift};
        if (e.key == Key::Char) k.shift = false; // shift is already reflected in `ch`
        return k;
    }
    auto operator<=>(const KeyChord&) const = default;
};

using ActionFactory = std::function<std::vector<Action>(const KeyEvent&)>;
using KeyPredicate = std::function<bool(const KeyEvent&)>;

// Data-driven key map: (mode, chord) -> actions. Resolution order:
//   1. exact binding in the active mode
//   2. exact binding in the global scope ("*")
//   3. fallback predicates of the active mode (e.g. "any printable character")
class KeyBindingRegistry {
public:
    void bind(std::string_view mode, KeyChord chord, ActionFactory factory);
    void bind(std::string_view mode, KeyChord chord, Action action);
    void bind(std::string_view mode, KeyChord chord, std::vector<Action> sequence);
    void bindFallback(std::string_view mode, KeyPredicate when, ActionFactory factory);
    bool unbind(std::string_view mode, KeyChord chord);

    // Zero, one, or several actions for a key press.
    std::vector<Action> resolve(const ModeId& mode, const KeyEvent& event) const;

    static bool isPrintable(const KeyEvent& e) { return e.key == Key::Char && !e.ctrl && !e.alt && e.ch >= 0x20 && e.ch < 0x7f; }

private:
    struct Fallback { KeyPredicate when; ActionFactory make; };
    std::map<std::string, std::map<KeyChord, ActionFactory>, std::less<>> exact_;
    std::map<std::string, std::vector<Fallback>, std::less<>> fallbacks_;
};

} // namespace ced
