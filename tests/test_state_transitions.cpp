#include <cctype>
#include "MockTerminal.h"
#include "TestFramework.h"
#include "controller/MainController.h"
#include "datasource/MemoryDataSource.h"
#include "state/CommonHandlers.h"

using namespace ced;
using ced::test::MockTerminal;

namespace {
struct Editor {
    MockTerminal term;
    MainController c;
    MemoryDataSource* src;
    explicit Editor(const std::string& content = "", int w = 40, int h = 8) : term(w, h), c(term), src(nullptr) {
        auto s = std::make_unique<MemoryDataSource>(content);
        src = s.get();
        std::string err;
        c.openSource(std::move(s), err);
        c.renderNow();
    }
    void key(KeyEvent e) { c.processEvent(InputEvent::fromKey(e)); }
    void type(const std::string& s) {
        for (char ch : s) key(ch == '\n' ? KeyEvent::special(Key::Enter) : KeyEvent::character(ch));
    }
    void esc() { key(KeyEvent::special(Key::Escape)); }
    std::string text() { return c.backend().readRange(0, c.backend().size()); }
};
} // namespace

TEST(state_transitions, starts_in_normal_mode_where_text_is_not_inserted) {
    Editor e("abc");
    CHECK_EQ(e.c.activeMode(), std::string("normal"));
    e.type("q");
    CHECK_EQ(e.text(), std::string("abc"));
}

TEST(state_transitions, insert_mode_typing_and_newline) {
    Editor e;
    e.type("i");
    CHECK_EQ(e.c.activeMode(), std::string("insert"));
    e.type("ab\ncd");
    CHECK_EQ(e.text(), std::string("ab\ncd"));
    CHECK_EQ(e.c.view().cursorLine, 1u);
    CHECK_EQ(e.c.view().cursorCol, 2u);
    e.esc();
    CHECK_EQ(e.c.activeMode(), std::string("normal"));
}

TEST(state_transitions, same_action_different_meaning_per_mode) {
    Editor e("abc\ndef");
    e.key(KeyEvent::special(Key::Enter)); // normal: Enter moves down, does not insert
    CHECK_EQ(e.text(), std::string("abc\ndef"));
    CHECK_EQ(e.c.view().cursorLine, 1u);
}

TEST(state_transitions, append_open_line_sequences) {
    Editor e("ab");
    e.type("A");                 // [MoveLineEnd, SwitchMode insert]
    e.type("!");
    CHECK_EQ(e.text(), std::string("ab!"));
    e.esc();
    e.type("o");                 // [MoveLineEnd, insert, InsertNewline]
    e.type("x");
    CHECK_EQ(e.text(), std::string("ab!\nx"));
}

TEST(state_transitions, backspace_joins_lines_and_respects_start) {
    Editor e("ab\ncd");
    e.key(KeyEvent::special(Key::Down));
    e.type("i");
    e.key(KeyEvent::special(Key::Backspace));
    CHECK_EQ(e.text(), std::string("abcd"));
    CHECK_EQ(e.c.view().cursorLine, 0u);
    CHECK_EQ(e.c.view().cursorCol, 2u);
    e.key(KeyEvent::special(Key::Home));
    e.key(KeyEvent::special(Key::Backspace)); // at document start: no-op
    CHECK_EQ(e.text(), std::string("abcd"));
}

TEST(state_transitions, cursor_boundaries) {
    Editor e("ab\nc");
    for (int i = 0; i < 5; ++i) e.key(KeyEvent::special(Key::Up));
    for (int i = 0; i < 5; ++i) e.key(KeyEvent::special(Key::Left));
    CHECK_EQ(e.c.view().cursorLine, 0u);
    CHECK_EQ(e.c.view().cursorCol, 0u);
    for (int i = 0; i < 10; ++i) e.key(KeyEvent::special(Key::Right));
    CHECK_EQ(e.c.view().cursorLine, 1u);
    CHECK_EQ(e.c.view().cursorCol, 1u);
    for (int i = 0; i < 5; ++i) e.key(KeyEvent::special(Key::Down));
    CHECK_EQ(e.c.view().cursorLine, 1u);
}

TEST(state_transitions, vertical_movement_remembers_column) {
    Editor e("long line\nx\nlong line");
    e.type("lllll");
    CHECK_EQ(e.c.view().cursorCol, 5u);
    e.type("j");
    CHECK_EQ(e.c.view().cursorLine, 1u);
    CHECK_EQ(e.c.view().cursorCol, 1u); // clamped to the short line...
    e.type("j");
    CHECK_EQ(e.c.view().cursorLine, 2u);
    CHECK_EQ(e.c.view().cursorCol, 5u); // ...but the desired column is remembered
}

TEST(state_transitions, undo_redo_through_keys) {
    Editor e("abc");
    e.type("A");
    e.type("de");
    e.esc();
    e.type("u");
    CHECK_EQ(e.text(), std::string("abcd"));
    e.key(KeyEvent::ctrlChar('r'));
    CHECK_EQ(e.text(), std::string("abcde"));
    e.key(KeyEvent::ctrlChar('z'));
    e.key(KeyEvent::ctrlChar('z'));
    e.key(KeyEvent::ctrlChar('z')); // nothing left: fails safely
    CHECK_EQ(e.text(), std::string("abc"));
    CHECK(e.c.statusMessage().find("Nothing to undo") != std::string::npos);
}

TEST(state_transitions, command_mode_write_and_quit_rules) {
    Editor e("abc");
    e.type("x");                    // delete 'a'
    CHECK(e.c.backend().isModified());
    e.type(":q\n");                 // refused: unsaved
    CHECK(!e.c.quitRequested());
    CHECK_EQ(e.c.activeMode(), std::string("normal"));
    e.type(":w\n");
    CHECK_EQ(e.src->content(), std::string("bc"));
    CHECK(!e.c.backend().isModified());
    e.type(":q\n");
    CHECK(e.c.quitRequested());
}

TEST(state_transitions, command_mode_edge_cases) {
    Editor e("abc");
    e.type(":bogus\n");
    CHECK(e.c.statusMessage().find("Unknown command") != std::string::npos);
    e.type(":ab");
    CHECK_EQ(e.c.activeMode(), std::string("command"));
    e.key(KeyEvent::special(Key::Backspace));
    e.key(KeyEvent::special(Key::Backspace));
    e.key(KeyEvent::special(Key::Backspace)); // empty buffer + backspace leaves command mode
    CHECK_EQ(e.c.activeMode(), std::string("normal"));
    e.type("x");
    e.type(":q!\n");
    CHECK(e.c.quitRequested());
}

TEST(state_transitions, save_failure_reported_and_document_stays_modified) {
    Editor e("abc");
    e.type("x");
    e.src->setWriteFailure(true);
    e.key(KeyEvent::ctrlChar('s'));
    CHECK(e.c.statusMessage().find("Save failed") != std::string::npos);
    CHECK(e.c.backend().isModified());
}

TEST(state_transitions, new_mode_registers_without_modifying_existing_states) {
    // A new "overwrite-lite" mode defined entirely outside the library.
    class UpperState final : public HandlerTableState {
    public:
        UpperState() {
            on(actions::InsertCharacter, [](const Action& a, EditorContext& ctx) {
                std::string up = *a.text();
                for (char& c : up) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                return handlers::insertCharacter(Action::withText(actions::InsertCharacter, up), ctx);
            });
            on(actions::SwitchMode, handlers::switchMode);
        }
        ModeId modeId() const override { return "upper"; }
    };
    Editor e;
    e.c.states().add(std::make_unique<UpperState>());
    e.c.keys().bind("normal", KeyChord::chr('U'), Action::switchMode("upper"));
    e.c.keys().bindFallback("upper", KeyBindingRegistry::isPrintable, [](const KeyEvent& k) {
        return std::vector<Action>{Action::withText(actions::InsertCharacter, std::string(1, k.ch))};
    });
    e.type("U");
    CHECK_EQ(e.c.activeMode(), std::string("upper"));
    e.type("abc");
    CHECK_EQ(e.text(), std::string("ABC"));
    // Navigation is not registered in this mode: the action is simply unhandled.
    e.key(KeyEvent::special(Key::Left));
    CHECK_EQ(e.text(), std::string("ABC"));
}

TEST(state_transitions, unknown_mode_and_unhandled_actions_fail_safely) {
    Editor e("abc");
    e.c.keys().bind("normal", KeyChord::chr('Z'), Action::switchMode("does-not-exist"));
    e.type("Z");
    CHECK_EQ(e.c.activeMode(), std::string("normal"));
    e.c.keys().bind("normal", KeyChord::chr('Y'), Action::simple("NoSuchAction"));
    e.type("Y");
    CHECK_EQ(e.text(), std::string("abc"));
}

TEST(state_transitions, viewport_scrolls_with_cursor) {
    std::string doc;
    for (int i = 0; i < 30; ++i) doc += "line" + std::to_string(i) + "\n";
    Editor e(doc, 40, 8); // 7 text rows
    for (int i = 0; i < 10; ++i) e.type("j");
    CHECK_EQ(e.c.view().cursorLine, 10u);
    CHECK(e.c.view().firstVisibleLine == 4u);
    CHECK_EQ(e.term.rowText(6), std::string("line10"));
    e.key(KeyEvent::special(Key::PageDown));
    CHECK(e.c.view().cursorLine >= e.c.view().firstVisibleLine);
    CHECK(e.c.view().cursorLine <= e.c.view().lastVisibleLine());
}

TEST(state_transitions, edits_rejected_when_document_not_ready) {
    MockTerminal term;
    MainController c(term);
    class Broken final : public IDataSource {
    public:
        std::string name() const override { return "broken"; }
        bool exists() const override { return true; }
        bool open(std::string& e) override { e = "boom"; return false; }
        std::optional<std::uint64_t> size() const override { return std::nullopt; }
        bool read(std::uint64_t, char*, std::size_t, std::size_t&, std::string&) override { return false; }
        bool supportsWrite() const override { return false; }
        bool beginWrite(std::string&) override { return false; }
        bool writeChunk(std::string_view, std::string&) override { return false; }
        bool commitWrite(std::string&) override { return false; }
        void abortWrite() override {}
    };
    std::string err;
    CHECK(!c.openSource(std::make_unique<Broken>(), err));
    CHECK_EQ(err, std::string("boom"));
    c.processEvent(InputEvent::fromKey(KeyEvent::character('i')));
    c.processEvent(InputEvent::fromKey(KeyEvent::character('x')));
    CHECK_EQ(c.backend().size(), 0u);
    CHECK(c.backend().status() == DocumentStatus::Error);
}
