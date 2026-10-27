#include "TestFramework.h"
#include "datasource/MemoryDataSource.h"
#include "document/TextProcessBackend.h"

#include <random>

using namespace ced;

namespace {
struct Doc {
    TextProcessBackend b;
    explicit Doc(const std::string& content = "") {
        std::string err;
        b.open(std::make_unique<MemoryDataSource>(content), err);
    }
    std::string text() const { return b.readRange(0, b.size()); }
};
} // namespace

TEST(history, undo_redo_insert_and_delete) {
    Doc d("hello");
    d.b.insert(5, " world");
    d.b.erase(0, 1);
    CHECK_EQ(d.text(), std::string("ello world"));
    CHECK(d.b.undo());
    CHECK_EQ(d.text(), std::string("hello world"));
    CHECK(d.b.undo());
    CHECK_EQ(d.text(), std::string("hello"));
    CHECK(!d.b.undo());
    CHECK(d.b.redo());
    CHECK(d.b.redo());
    CHECK_EQ(d.text(), std::string("ello world"));
    CHECK(!d.b.redo());
}

TEST(history, new_edit_clears_redo) {
    Doc d("a");
    d.b.insert(1, "b");
    d.b.undo();
    CHECK(d.b.canRedo());
    d.b.insert(1, "c");
    CHECK(!d.b.canRedo());
    CHECK_EQ(d.text(), std::string("ac"));
}

TEST(history, failed_edits_are_not_recorded) {
    Doc d("abc");
    CHECK(!d.b.insert(10, "x"));
    CHECK(!d.b.erase(2, 5));
    CHECK(!d.b.canUndo());
    CHECK(!d.b.isModified());
}

TEST(history, grouped_operations_undo_as_one) {
    Doc d("abc");
    d.b.beginGroup();
    d.b.insert(3, "d");
    d.b.insert(4, "e");
    d.b.erase(0, 1);
    d.b.endGroup();
    CHECK_EQ(d.text(), std::string("bcde"));
    CHECK(d.b.undo().has_value());
    CHECK_EQ(d.text(), std::string("abc"));
    CHECK(!d.b.canUndo());
    d.b.redo();
    CHECK_EQ(d.text(), std::string("bcde"));
}

TEST(history, replace_is_one_step) {
    Doc d("hello world");
    d.b.replace(6, 5, "there");
    CHECK_EQ(d.text(), std::string("hello there"));
    d.b.undo();
    CHECK_EQ(d.text(), std::string("hello world"));
    CHECK(!d.b.canUndo());
}

TEST(history, cursor_hints) {
    Doc d("abc");
    d.b.insert(1, "XY");
    auto u = d.b.undo();
    CHECK(u && u->cursorOffset == 1u);
    auto r = d.b.redo();
    CHECK(r && r->cursorOffset == 3u);
    d.b.erase(1, 2);
    auto u2 = d.b.undo();
    CHECK(u2 && u2->cursorOffset == 3u);
}

TEST(history, modified_flag_tracks_save_point) {
    MemoryDataSource* src = new MemoryDataSource("abc");
    TextProcessBackend b;
    std::string err;
    b.open(std::unique_ptr<IDataSource>(src), err);
    CHECK(!b.isModified());
    b.insert(3, "d");
    CHECK(b.isModified());
    CHECK(b.save(err));
    CHECK(!b.isModified());
    b.insert(4, "e");
    b.undo();
    CHECK(!b.isModified()); // back at the saved state
    b.undo();
    CHECK(b.isModified());
    b.insert(0, "z"); // discards the redo branch that contained the saved state
    b.undo();
    b.redo();
    CHECK(b.isModified());
}

TEST(history, failed_save_keeps_modified) {
    MemoryDataSource* src = new MemoryDataSource("abc");
    TextProcessBackend b;
    std::string err;
    b.open(std::unique_ptr<IDataSource>(src), err);
    b.insert(0, "x");
    src->setWriteFailure(true);
    CHECK(!b.save(err));
    CHECK(!err.empty());
    CHECK(b.isModified());
    CHECK_EQ(src->content(), std::string("abc")); // original untouched
    src->setWriteFailure(false);
    CHECK(b.save(err));
    CHECK(!b.isModified());
}

TEST(history, randomized_undo_all_then_redo_all) {
    std::mt19937 rng(4242);
    std::string base;
    for (int i = 0; i < 3000; ++i) base.push_back(rng() % 9 == 0 ? '\n' : static_cast<char>('a' + rng() % 26));
    Doc d(base);
    std::vector<std::string> snapshots{base};
    for (int i = 0; i < 400; ++i) {
        std::string cur = d.text();
        if (rng() % 2 || cur.empty()) {
            d.b.insert(rng() % (cur.size() + 1), rng() % 4 ? "xyz" : "q\nr");
        } else {
            std::size_t pos = rng() % cur.size();
            d.b.erase(pos, std::min<std::size_t>(1 + rng() % 12, cur.size() - pos));
        }
        snapshots.push_back(d.text());
    }
    for (std::size_t i = snapshots.size() - 1; i-- > 0;) {
        CHECK(d.b.undo());
        CHECK_EQ(d.text() == snapshots[i], true);
    }
    CHECK(!d.b.canUndo());
    for (std::size_t i = 1; i < snapshots.size(); ++i) {
        CHECK(d.b.redo());
        CHECK_EQ(d.text() == snapshots[i], true);
    }
    CHECK(d.b.table().validate());
}
