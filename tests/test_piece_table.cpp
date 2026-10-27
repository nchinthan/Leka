#include "TestFramework.h"
#include "document/AugmentedPieceTable.h"

#include <random>

using namespace ced;

namespace {
struct Fixture {
    BufferStore bs;
    AugmentedPieceTable t{bs};
    explicit Fixture(const std::string& original = "") {
        bs.original().append(original);
        t.initFromOriginal();
    }
};
} // namespace

TEST(piece_table, empty_document) {
    Fixture f;
    CHECK_EQ(f.t.size(), 0u);
    CHECK_EQ(f.t.lineCount(), 1u);
    CHECK_EQ(f.t.read(0, 10), std::string());
    CHECK(f.t.validate());
}

TEST(piece_table, insert_begin_middle_end) {
    Fixture f("bd");
    CHECK(f.t.insert(1, "c"));
    CHECK(f.t.insert(0, "a"));
    CHECK(f.t.insert(f.t.size(), "e"));
    CHECK_EQ(f.t.read(0, f.t.size()), std::string("abcde"));
    CHECK(f.t.validate());
}

TEST(piece_table, insert_into_empty) {
    Fixture f;
    CHECK(f.t.insert(0, "hello"));
    CHECK_EQ(f.t.read(0, 5), std::string("hello"));
    CHECK(!f.t.insert(99, "x")); // out of range rejected
    CHECK_EQ(f.t.size(), 5u);
}

TEST(piece_table, delete_within_and_across_pieces) {
    Fixture f("0123456789");
    f.t.insert(5, "ABC"); // 01234ABC56789
    CHECK(f.t.erase(2, 2)); // 014ABC56789
    CHECK_EQ(f.t.read(0, f.t.size()), std::string("014ABC56789"));
    CHECK(f.t.erase(2, 6)); // across original/add/original pieces
    CHECK_EQ(f.t.read(0, f.t.size()), std::string("01789"));
    CHECK(!f.t.erase(3, 10));
    CHECK(f.t.validate());
}

TEST(piece_table, typing_run_coalesces) {
    Fixture g;
    for (int i = 0; i < 100; ++i) g.t.insert(g.t.size(), "k");
    CHECK_EQ(g.t.pieceCount(), 1u);
    CHECK_EQ(g.t.size(), 100u);
}

TEST(piece_table, erase_recoalesces_split_pieces) {
    Fixture f("abcdef");
    f.t.insert(3, "XYZ");
    CHECK_EQ(f.t.pieceCount(), 3u);
    f.t.erase(3, 3); // remove the inserted text: halves of the original piece are contiguous again
    CHECK_EQ(f.t.pieceCount(), 1u);
    CHECK_EQ(f.t.read(0, 6), std::string("abcdef"));
    CHECK(f.t.validate());
}

TEST(piece_table, aggregates_and_newlines) {
    Fixture f("a\nb\n");
    CHECK_EQ(f.t.lineBreakCount(), 2u);
    f.t.insert(2, "x\ny\n");
    CHECK_EQ(f.t.lineBreakCount(), 4u);
    f.t.erase(0, 4);
    CHECK_EQ(f.t.read(0, f.t.size()), std::string("y\nb\n"));
    CHECK_EQ(f.t.lineBreakCount(), 2u);
    CHECK(f.t.validate());
}

TEST(piece_table, randomized_against_reference) {
    std::mt19937 rng(12345);
    // Original spans multiple 64 KiB chunks so cross-chunk paths are exercised.
    std::string ref;
    for (std::size_t i = 0; i < 150000; ++i) ref.push_back(rng() % 17 == 0 ? '\n' : static_cast<char>('a' + rng() % 26));
    Fixture f(ref);

    for (int step = 0; step < 6000; ++step) {
        const bool doInsert = ref.empty() || rng() % 2 == 0;
        if (doInsert) {
            const std::size_t pos = rng() % (ref.size() + 1);
            std::string text;
            for (std::size_t n = 1 + rng() % 40; n > 0; --n) text.push_back(rng() % 6 == 0 ? '\n' : static_cast<char>('A' + rng() % 26));
            CHECK(f.t.insert(pos, text));
            ref.insert(pos, text);
        } else {
            const std::size_t pos = rng() % ref.size();
            const std::size_t len = std::min<std::size_t>(1 + rng() % (step % 50 == 0 ? 3000 : 30), ref.size() - pos);
            CHECK(f.t.erase(pos, len));
            ref.erase(pos, len);
        }
        if (step % 250 == 0) {
            CHECK(f.t.validate());
            CHECK_EQ(f.t.size(), ref.size());
            const std::size_t a = rng() % (ref.size() + 1), n = rng() % 500;
            CHECK_EQ(f.t.read(a, n), ref.substr(a, n));
        }
    }
    CHECK_EQ(f.t.read(0, f.t.size()), ref);
    CHECK(f.t.validate());
    // Tree stayed balanced enough that piece count is far below operation count * constant.
    CHECK(f.t.pieceCount() < 20000u);
}
