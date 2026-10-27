#pragma once
#include "renderer/ITerminal.h"

#include <deque>
#include <string>
#include <vector>

namespace ced::test {

// Records every output operation and mirrors the result into a character grid.
class MockTerminal final : public ITerminal {
public:
    struct Op { enum Kind { Write, Clear, ClearScreen, Cursor } kind; int row, col; std::string text; int length; };

    MockTerminal(int w = 40, int h = 8) : w_(w), h_(h) { reset(); }

    void size(int& width, int& height) const override { width = w_; height = h_; }
    void write(int row, int col, std::string_view text) override {
        ops.push_back({Op::Write, row, col, std::string(text), static_cast<int>(text.size())});
        for (std::size_t i = 0; i < text.size(); ++i) grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(col) + i] = text[i];
    }
    void clearRange(int row, int col, int length) override {
        ops.push_back({Op::Clear, row, col, {}, length});
        for (int i = 0; i < length; ++i) grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(col + i)] = ' ';
    }
    void clearScreen() override { ops.push_back({Op::ClearScreen, 0, 0, {}, 0}); reset(); }
    void setCursor(int row, int col) override { cursorRow = row; cursorCol = col; ops.push_back({Op::Cursor, row, col, {}, 0}); }
    void flush() override {}
    InputEvent readEvent() override {
        if (input.empty()) return InputEvent::closed();
        InputEvent e = input.front();
        input.pop_front();
        return e;
    }

    // test helpers
    void resize(int w, int h) { w_ = w; h_ = h; reset(); }
    void reset() { grid.assign(static_cast<std::size_t>(h_), std::string(static_cast<std::size_t>(w_), ' ')); }
    std::string rowText(int r) const {
        std::string s = grid[static_cast<std::size_t>(r)];
        s.erase(s.find_last_not_of(' ') == std::string::npos ? 0 : s.find_last_not_of(' ') + 1);
        return s;
    }
    std::size_t cellsWritten() const { std::size_t n = 0; for (auto& o : ops) if (o.kind == Op::Write || o.kind == Op::Clear) n += static_cast<std::size_t>(o.length); return n; }
    std::size_t opsOnRowsBelow(int row) const { std::size_t n = 0; for (auto& o : ops) if ((o.kind == Op::Write || o.kind == Op::Clear) && o.row >= row) ++n; return n; }
    std::size_t opsOnRowsAtOrAbove(int row) const { std::size_t n = 0; for (auto& o : ops) if ((o.kind == Op::Write || o.kind == Op::Clear) && o.row <= row) ++n; return n; }
    void clearLog() { ops.clear(); }

    std::vector<Op> ops;
    std::vector<std::string> grid;
    std::deque<InputEvent> input;
    int cursorRow = 0, cursorCol = 0;

private:
    int w_, h_;
};

} // namespace ced::test
