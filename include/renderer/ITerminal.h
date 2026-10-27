#pragma once
#include "input/InputEvent.h"

#include <string_view>

namespace ced {

// All platform-specific console access goes through this interface.
// Coordinates are zero-based (row, column).
class ITerminal {
public:
    virtual ~ITerminal() = default;

    virtual void size(int& width, int& height) const = 0;
    virtual void write(int row, int col, std::string_view text) = 0;  // does not move the logical cursor
    virtual void clearRange(int row, int col, int length) = 0;        // fill with spaces
    virtual void clearScreen() = 0;
    virtual void setCursor(int row, int col) = 0;
    virtual void flush() = 0;
    virtual InputEvent readEvent() = 0;                                // blocks; Closed when input ends
};

} // namespace ced
