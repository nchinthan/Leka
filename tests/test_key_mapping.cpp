#include "TestFramework.h"
#include "input/KeyMap.h"

using namespace ced;

TEST(key_mapping, mode_specific_and_global_and_fallback) {
    KeyBindingRegistry r;
    installDefaultKeyBindings(r);

    auto a = r.resolve("insert", KeyEvent::character('x'));
    CHECK_EQ(a.size(), 1u);
    CHECK(a[0].is(actions::InsertCharacter));
    CHECK_EQ(*a[0].text(), std::string("x"));

    CHECK(r.resolve("normal", KeyEvent::character('x'))[0].is(actions::DeleteForward)); // same key, other meaning
    CHECK(r.resolve("normal", KeyEvent::character('q')).empty());                       // unbound => no action
    CHECK(r.resolve("insert", KeyEvent::special(Key::Left))[0].is(actions::MoveCursorLeft)); // global
    CHECK(r.resolve("command", KeyEvent::ctrlChar('s'))[0].is(actions::SaveDocument));
}

TEST(key_mapping, sequences) {
    KeyBindingRegistry r;
    installDefaultKeyBindings(r);
    auto o = r.resolve("normal", KeyEvent::character('o'));
    CHECK_EQ(o.size(), 3u);
    CHECK(o[2].is(actions::InsertNewline));
}

TEST(key_mapping, rebinding_without_touching_other_modules) {
    KeyBindingRegistry r;
    installDefaultKeyBindings(r);
    r.bind("normal", KeyChord::chr('x'), Action::simple(actions::MoveCursorRight));
    CHECK(r.resolve("normal", KeyEvent::character('x'))[0].is(actions::MoveCursorRight));
    CHECK(r.unbind("normal", KeyChord::chr('x')));
    CHECK(r.resolve("normal", KeyEvent::character('x')).empty());
}

TEST(key_mapping, ctrl_chars_are_not_printable_text) {
    KeyBindingRegistry r;
    installDefaultKeyBindings(r);
    CHECK(r.resolve("insert", KeyEvent::ctrlChar('q')).empty());
}
