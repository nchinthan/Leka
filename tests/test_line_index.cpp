#include "TestFramework.h"
#include "datasource/MemoryDataSource.h"
#include "document/TextProcessBackend.h"

#include <random>

using namespace ced;

namespace {
std::unique_ptr<TextProcessBackend> make(const std::string& content, MemoryDataSource** out = nullptr) {
    auto b = std::make_unique<TextProcessBackend>();
    auto src = std::make_unique<MemoryDataSource>(content);
    if (out) *out = src.get();
    std::string err;
    b->open(std::move(src), err);
    return b;
}
std::vector<std::size_t> refLineStarts(const std::string& s) {
    std::vector<std::size_t> v{0};
    for (std::size_t i = 0; i < s.size(); ++i) if (s[i] == '\n') v.push_back(i + 1);
    return v;
}
} // namespace

TEST(line_index, empty_and_trailing_newline) {
    auto e = make("");
    CHECK_EQ(e->lineCount(), 1u);
    CHECK_EQ(e->lineLength(0), 0u);
    auto t = make("a\nb\n");
    CHECK_EQ(t->lineCount(), 3u); // trailing newline => final empty line
    CHECK_EQ(t->lineText(1), std::string("b"));
    CHECK_EQ(t->lineText(2), std::string());
    CHECK_EQ(t->lineStartOffset(2), 4u);
}

TEST(line_index, offset_and_start_roundtrip) {
    auto b = make("one\n\nthree\nfour");
    CHECK_EQ(b->lineCount(), 4u);
    CHECK_EQ(b->lineText(1), std::string());
    CHECK_EQ(b->lineOfOffset(0), 0u);
    CHECK_EQ(b->lineOfOffset(3), 0u); // the '\n' belongs to line 0
    CHECK_EQ(b->lineOfOffset(4), 1u);
    CHECK_EQ(b->lineOfOffset(5), 2u);
    CHECK_EQ(b->lineOfOffset(b->size()), 3u);
    CHECK_EQ(b->lineStartOffset(99), b->size()); // clamps
}

TEST(line_index, crlf_normalized_and_restored_on_save) {
    MemoryDataSource* src = nullptr;
    auto b = make("a\r\nb\r\nc", &src);
    CHECK_EQ(b->lineCount(), 3u);
    CHECK_EQ(b->readRange(0, b->size()), std::string("a\nb\nc"));
    CHECK(b->lineEnding() == LineEnding::CRLF);
    b->insert(b->size(), "!");
    std::string err;
    CHECK(b->save(err));
    CHECK_EQ(src->content(), std::string("a\r\nb\r\nc!"));
}

TEST(line_index, lone_cr_and_crlf_across_chunk_boundary) {
    std::string s(ChunkedBuffer::kChunkSize - 1, 'x');
    s += "\r\nend\rq";
    auto b = make(s);
    CHECK_EQ(b->lineCount(), 2u);
    CHECK_EQ(b->lineText(1), std::string("end\rq")); // lone CR preserved as data
}

TEST(line_index, lf_file_stays_lf) {
    MemoryDataSource* src = nullptr;
    auto b = make("a\nb\n", &src);
    CHECK(b->lineEnding() == LineEnding::LF);
    std::string err;
    CHECK(b->save(err));
    CHECK_EQ(src->content(), std::string("a\nb\n"));
}

TEST(line_index, randomized_metadata_matches_reference) {
    std::mt19937 rng(777);
    std::string ref;
    for (int i = 0; i < 100000; ++i) ref.push_back(rng() % 11 == 0 ? '\n' : 'a');
    auto b = make(ref);
    for (int step = 0; step < 2000; ++step) {
        if (rng() % 2) {
            std::size_t pos = rng() % (ref.size() + 1);
            std::string t = rng() % 3 ? "zz" : "\n\n";
            b->insert(pos, t);
            ref.insert(pos, t);
        } else if (!ref.empty()) {
            std::size_t pos = rng() % ref.size(), len = std::min<std::size_t>(1 + rng() % 20, ref.size() - pos);
            b->erase(pos, len);
            ref.erase(pos, len);
        }
        if (step % 100 == 0) {
            auto starts = refLineStarts(ref);
            CHECK_EQ(b->lineCount(), starts.size());
            for (int k = 0; k < 20; ++k) {
                std::size_t line = rng() % starts.size();
                CHECK_EQ(b->lineStartOffset(line), starts[line]);
                std::size_t off = starts[line] + (rng() % (b->lineLength(line) + 1));
                CHECK_EQ(b->lineOfOffset(off), line);
            }
        }
    }
    CHECK(b->table().validate());
}
