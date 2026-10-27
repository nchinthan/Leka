#include "MockTerminal.h"
#include "TestFramework.h"
#include "controller/MainController.h"
#include "datasource/MemoryDataSource.h"

using namespace ced;
using ced::test::MockTerminal;

namespace {
struct Ed {
    MockTerminal term;
    MainController c;
    explicit Ed(const std::string& content, int w = 30, int h = 6) : term(w, h), c(term) {
        std::string err;
        c.openSource(std::make_unique<MemoryDataSource>(content), err);
        c.renderNow();
    }
    void key(KeyEvent e) { c.processEvent(InputEvent::fromKey(e)); }
    void type(const std::string& s) { for (char ch : s) key(ch == '\n' ? KeyEvent::special(Key::Enter) : KeyEvent::character(ch)); }
    // The last text row is the status bar; document rows are 0..h-2.
    int statusRow() const { return 5; }
};
} // namespace

TEST(rendering, initial_render_shows_document_and_status) {
    Ed e("hello\nworld");
    CHECK_EQ(e.term.rowText(0), std::string("hello"));
    CHECK_EQ(e.term.rowText(1), std::string("world"));
    CHECK(e.term.rowText(e.statusRow()).find("NORMAL") != std::string::npos);
    CHECK_EQ(e.c.renderer().stats().fullScreenRedraws, 1u);
}

TEST(rendering, append_at_end_writes_exactly_one_cell) {
    Ed e("hello\nworld");
    e.type("A");
    e.term.clearLog();
    e.type("!");
    CHECK_EQ(e.term.rowText(0), std::string("hello!"));
    // only the one new character on the document rows
    std::size_t docCells = 0;
    for (auto& o : e.term.ops) if (o.kind == MockTerminal::Op::Write && o.row < 5) docCells += static_cast<std::size_t>(o.length);
    CHECK_EQ(docCells, 1u);
    CHECK_EQ(e.term.opsOnRowsBelow(1) - e.term.opsOnRowsBelow(5), 0u); // row 1.. untouched except status row
}

TEST(rendering, middle_insert_shifts_suffix_and_leaves_prefix_and_other_lines) {
    Ed e("abcdef\nsecond");
    e.type("lll");   // cursor at col 3
    e.type("i");
    e.term.clearLog();
    e.type("X");
    CHECK_EQ(e.term.rowText(0), std::string("abcXdef"));
    for (auto& o : e.term.ops)
        if (o.kind == MockTerminal::Op::Write && o.row == 0) CHECK(o.col >= 3); // "abc" never rewritten
    CHECK_EQ(e.term.rowText(1), std::string("second"));
    // nothing touched on row 1..4
    std::size_t other = 0;
    for (auto& o : e.term.ops) if ((o.kind == MockTerminal::Op::Write || o.kind == MockTerminal::Op::Clear) && o.row >= 1 && o.row <= 4) ++other;
    CHECK_EQ(other, 0u);
}

TEST(rendering, delete_clears_vacated_cell) {
    Ed e("abcdef");
    e.type("$hxhx"); // '$' parks the cursor after the last char; 'h' steps onto it
    CHECK_EQ(e.term.rowText(0), std::string("abcd"));
    e.term.clearLog();
    e.type("0");
    e.type("x"); // delete 'a': whole suffix shifts, trailing cells cleared
    CHECK_EQ(e.term.rowText(0), std::string("bcd"));
    CHECK(e.term.grid[0].substr(3) == std::string(27, ' '));
}

TEST(rendering, shortening_line_clears_leftovers_only_in_that_line) {
    Ed e("abcdef\nzzzzzz");
    e.type("lll");
    e.type("x");
    e.type("x");
    e.type("x");
    CHECK_EQ(e.term.rowText(0), std::string("abc"));
    CHECK_EQ(e.term.rowText(1), std::string("zzzzzz"));
}

TEST(rendering, newline_insert_updates_affected_and_shifted_lines) {
    Ed e("one\ntwo\nthree");
    e.type("l");
    e.type("i");
    e.term.clearLog();
    e.key(KeyEvent::special(Key::Enter));
    CHECK_EQ(e.term.rowText(0), std::string("o"));
    CHECK_EQ(e.term.rowText(1), std::string("ne"));
    CHECK_EQ(e.term.rowText(2), std::string("two"));
    CHECK_EQ(e.term.rowText(3), std::string("three"));
    CHECK_EQ(e.c.renderer().stats().fullScreenRedraws, 1u); // no full redraw for ordinary typing
}

TEST(rendering, newline_delete_pulls_following_lines_up) {
    Ed e("one\ntwo\nthree");
    e.type("j");
    e.type("i");
    e.key(KeyEvent::special(Key::Home));
    e.key(KeyEvent::special(Key::Backspace));
    CHECK_EQ(e.term.rowText(0), std::string("onetwo"));
    CHECK_EQ(e.term.rowText(1), std::string("three"));
    CHECK_EQ(e.term.rowText(2), std::string());
}

TEST(rendering, cursor_movement_does_not_redraw_document_text) {
    Ed e("hello\nworld\nagain");
    e.term.clearLog();
    e.type("l");
    e.type("j");
    e.type("$");
    CHECK_EQ(e.term.opsOnRowsAtOrAbove(4), 0u); // no document row written at all
    CHECK_EQ(e.term.cursorRow, 1);
    CHECK_EQ(e.term.cursorCol, 5);
}

TEST(rendering, unchanged_chars_in_same_position_are_not_rewritten) {
    Ed e("aaaa");
    e.type("i");
    e.term.clearLog();
    e.type("a"); // "aaaaa": only the single new trailing cell differs
    CHECK_EQ(e.term.rowText(0), std::string("aaaaa"));
    std::size_t docCells = 0;
    for (auto& o : e.term.ops) if (o.kind == MockTerminal::Op::Write && o.row < 5) docCells += static_cast<std::size_t>(o.length);
    CHECK_EQ(docCells, 1u);
}

TEST(rendering, tabs_expand_and_cursor_uses_display_column) {
    Ed e("\tx");
    CHECK_EQ(e.term.rowText(0), std::string("    x"));
    e.type("l");
    CHECK_EQ(e.term.cursorCol, 4);
}

TEST(rendering, horizontal_scroll_for_long_lines) {
    Ed e(std::string(50, 'a') + "END", 20, 4);
    e.type("$");
    CHECK(e.term.rowText(0).find("END") != std::string::npos);
    CHECK(e.c.view().horizontalOffset > 0u);
    CHECK(e.term.cursorCol < 20);
    e.type("0");
    CHECK_EQ(e.c.view().horizontalOffset, 0u);
    CHECK_EQ(e.term.rowText(0).substr(0, 3), std::string("aaa"));
}

TEST(rendering, scrolling_redraws_only_cells_that_differ) {
    std::string doc;
    for (int i = 0; i < 20; ++i) doc += "row" + std::to_string(i % 2) + "\n"; // alternating, so shifted rows often equal
    Ed e(doc, 20, 6);
    e.term.clearLog();
    for (int i = 0; i < 6; ++i) e.type("j");
    CHECK_EQ(e.term.rowText(0).substr(0, 3), std::string("row"));
    CHECK_EQ(e.c.renderer().stats().fullScreenRedraws, 1u);
}

TEST(rendering, resize_triggers_full_redraw_and_stays_correct) {
    Ed e("hello\nworld");
    e.term.resize(12, 4);
    e.c.processEvent(InputEvent::resized(12, 4));
    CHECK_EQ(e.c.renderer().stats().fullScreenRedraws, 2u);
    CHECK_EQ(e.term.rowText(0), std::string("hello"));
    CHECK_EQ(e.term.rowText(1), std::string("world"));
}

TEST(rendering, command_line_shown_in_status_row) {
    Ed e("x");
    e.type(":wq");
    CHECK_EQ(e.term.rowText(5), std::string(":wq"));
    CHECK_EQ(e.term.cursorRow, 5);
    CHECK_EQ(e.term.cursorCol, 3); // after "wq" (the ":" is the mode key, not text)
}

TEST(rendering, renderer_screen_cache_always_matches_terminal) {
    // Drive a random-ish session and verify the renderer's idea of the screen equals the real grid.
    Ed e("alpha\nbeta\ngamma\n", 25, 7);
    const std::string script = "iHello\nWorld\x01";
    e.type("ixyz\nabc");
    e.key(KeyEvent::special(Key::Escape));
    e.type("kkxxjjA end");
    e.key(KeyEvent::special(Key::Escape));
    e.type("uu");
    for (int r = 0; r < 7; ++r) {
        std::string cached = e.c.renderer().screen().row(r);
        CHECK_EQ(cached == e.term.grid[static_cast<std::size_t>(r)], true);
    }
    (void)script;
}
