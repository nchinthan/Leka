#pragma once
#include <cstddef>
#include <optional>

namespace ced {

struct Selection {
    std::size_t anchorLine, anchorCol;
    std::size_t activeLine, activeCol;
};

// Logical view/cursor state. Lines and columns are zero-based; columns count bytes of the line.
struct ViewState {
    std::size_t cursorLine = 0;
    std::size_t cursorCol = 0;
    std::size_t desiredCol = 0;         // remembered column for vertical movement
    std::size_t firstVisibleLine = 0;
    std::size_t horizontalOffset = 0;   // in display cells
    int terminalWidth = 80;
    int terminalHeight = 24;
    std::optional<Selection> selection; // reserved for a future selection feature

    // The bottom terminal row is the status bar.
    std::size_t textRows() const { return terminalHeight > 1 ? static_cast<std::size_t>(terminalHeight - 1) : 1; }
    std::size_t lastVisibleLine() const { return firstVisibleLine + textRows() - 1; }
};

} // namespace ced
