#pragma once
#include "renderer/ITerminal.h"

#ifdef _WIN32
#include <windows.h>

namespace ced {

// ITerminal over the Win32 Console API. Uses a private screen buffer, so the user's
// scrollback is untouched and restored on exit. Console text is written with
// WriteConsoleOutputCharacter (positioned writes, never scrolls).
// Input: ASCII printable keys, Ctrl+letter, arrows, Home/End, PgUp/PgDn, Enter, Tab,
// Backspace, Delete, Esc. Non-ASCII input is ignored (see README limitations).
class WindowsTerminal final : public ITerminal {
public:
    WindowsTerminal();
    ~WindowsTerminal() override;
    WindowsTerminal(const WindowsTerminal&) = delete;
    WindowsTerminal& operator=(const WindowsTerminal&) = delete;

    void size(int& width, int& height) const override;
    void write(int row, int col, std::string_view text) override;
    void clearRange(int row, int col, int length) override;
    void clearScreen() override;
    void setCursor(int row, int col) override;
    void flush() override {}
    InputEvent readEvent() override;

private:
    HANDLE in_ = INVALID_HANDLE_VALUE;
    HANDLE out_ = INVALID_HANDLE_VALUE;
    HANDLE previousOut_ = INVALID_HANDLE_VALUE;
    DWORD previousInMode_ = 0;
};

} // namespace ced
#endif
