#ifdef _WIN32
#include "renderer/WindowsTerminal.h"

#include <cctype>
#include <stdexcept>

namespace ced {

WindowsTerminal::WindowsTerminal() {
    in_ = GetStdHandle(STD_INPUT_HANDLE);
    previousOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
    if (in_ == INVALID_HANDLE_VALUE || previousOut_ == INVALID_HANDLE_VALUE)
        throw std::runtime_error("no console attached");

    out_ = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                     CONSOLE_TEXTMODE_BUFFER, nullptr);
    if (out_ == INVALID_HANDLE_VALUE) throw std::runtime_error("cannot create console screen buffer");

    GetConsoleMode(in_, &previousInMode_);
    // Raw key events: no line editing, no echo, no Ctrl+C processing, no QuickEdit selection.
    SetConsoleMode(in_, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS);
    SetConsoleActiveScreenBuffer(out_);
}

WindowsTerminal::~WindowsTerminal() {
    if (previousOut_ != INVALID_HANDLE_VALUE) SetConsoleActiveScreenBuffer(previousOut_);
    if (out_ != INVALID_HANDLE_VALUE) CloseHandle(out_);
    SetConsoleMode(in_, previousInMode_);
}

void WindowsTerminal::size(int& width, int& height) const {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(out_, &info)) {
        width = info.srWindow.Right - info.srWindow.Left + 1;
        height = info.srWindow.Bottom - info.srWindow.Top + 1;
    }
}

void WindowsTerminal::write(int row, int col, std::string_view text) {
    DWORD written = 0;
    WriteConsoleOutputCharacterA(out_, text.data(), static_cast<DWORD>(text.size()),
                                 COORD{static_cast<SHORT>(col), static_cast<SHORT>(row)}, &written);
}

void WindowsTerminal::clearRange(int row, int col, int length) {
    DWORD written = 0;
    FillConsoleOutputCharacterA(out_, ' ', static_cast<DWORD>(length),
                                COORD{static_cast<SHORT>(col), static_cast<SHORT>(row)}, &written);
}

void WindowsTerminal::clearScreen() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(out_, &info)) return;
    DWORD written = 0;
    FillConsoleOutputCharacterA(out_, ' ', static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y),
                                COORD{0, 0}, &written);
}

void WindowsTerminal::setCursor(int row, int col) {
    SetConsoleCursorPosition(out_, COORD{static_cast<SHORT>(col), static_cast<SHORT>(row)});
}

InputEvent WindowsTerminal::readEvent() {
    for (;;) {
        INPUT_RECORD rec;
        DWORD n = 0;
        if (!ReadConsoleInputW(in_, &rec, 1, &n) || n == 0) return InputEvent::closed();

        if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            int w = 80, h = 24;
            size(w, h);
            return InputEvent::resized(w, h);
        }
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) continue;

        const KEY_EVENT_RECORD& k = rec.Event.KeyEvent;
        const bool ctrl = (k.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        const bool alt = (k.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
        const bool shift = (k.dwControlKeyState & SHIFT_PRESSED) != 0;

        const auto special = [&](Key key) {
            KeyEvent e = KeyEvent::special(key);
            e.ctrl = ctrl;
            e.alt = alt;
            e.shift = shift;
            return InputEvent::fromKey(e);
        };

        switch (k.wVirtualKeyCode) {
        case VK_LEFT: return special(Key::Left);
        case VK_RIGHT: return special(Key::Right);
        case VK_UP: return special(Key::Up);
        case VK_DOWN: return special(Key::Down);
        case VK_HOME: return special(Key::Home);
        case VK_END: return special(Key::End);
        case VK_PRIOR: return special(Key::PageUp);
        case VK_NEXT: return special(Key::PageDown);
        case VK_RETURN: return special(Key::Enter);
        case VK_BACK: return special(Key::Backspace);
        case VK_DELETE: return special(Key::Delete);
        case VK_TAB: return special(Key::Tab);
        case VK_ESCAPE: return special(Key::Escape);
        default: break;
        }

        if (ctrl && !alt && k.wVirtualKeyCode >= 'A' && k.wVirtualKeyCode <= 'Z') {
            KeyEvent e = KeyEvent::ctrlChar(static_cast<char>(std::tolower(static_cast<int>(k.wVirtualKeyCode))));
            return InputEvent::fromKey(e);
        }
        const wchar_t c = k.uChar.UnicodeChar;
        if (c >= 0x20 && c < 0x7f) { // AltGr reports ctrl+alt; the produced character is still plain text
            if (ctrl && !alt) continue;
            return InputEvent::fromKey(KeyEvent::character(static_cast<char>(c)));
        }
        // Modifier-only presses, function keys, non-ASCII: ignore.
    }
}

} // namespace ced
#endif
