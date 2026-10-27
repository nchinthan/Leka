#pragma once

namespace ced {

enum class Key { Char, Enter, Backspace, Delete, Tab, Escape, Left, Right, Up, Down, Home, End, PageUp, PageDown };

// Normalized key press. Printable keys are Key::Char with `ch` already shifted;
// Ctrl+letter arrives as Key::Char with lower-case `ch` and ctrl=true.
struct KeyEvent {
    Key key = Key::Char;
    char ch = 0;
    bool ctrl = false;
    bool alt = false;
    bool shift = false;

    static KeyEvent character(char c) { KeyEvent e; e.ch = c; return e; }
    static KeyEvent ctrlChar(char c) { KeyEvent e; e.ch = c; e.ctrl = true; return e; }
    static KeyEvent special(Key k) { KeyEvent e; e.key = k; return e; }
};

struct InputEvent {
    enum class Type { Key, Resize, Closed };
    Type type = Type::Closed;
    KeyEvent key;
    int width = 0;
    int height = 0;

    static InputEvent fromKey(KeyEvent k) { InputEvent e; e.type = Type::Key; e.key = k; return e; }
    static InputEvent resized(int w, int h) { InputEvent e; e.type = Type::Resize; e.width = w; e.height = h; return e; }
    static InputEvent closed() { return {}; }
};

} // namespace ced
