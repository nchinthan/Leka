#pragma once
#include "document/Piece.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace ced {

// Append-only byte storage split into fixed-size chunks. Every chunk except the
// last is exactly kChunkSize bytes, so offset -> chunk is O(1). Each chunk caches
// its newline count, which lets line queries skip whole chunks.
// Offsets handed out remain valid forever (nothing is ever moved or removed).
class ChunkedBuffer {
public:
    static constexpr std::size_t kChunkSize = 64 * 1024;
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    std::size_t size() const { return size_; }

    void append(std::string_view text) {
        while (!text.empty()) {
            if (chunks_.empty() || chunks_.back().size() == kChunkSize) {
                chunks_.emplace_back();
                chunks_.back().reserve(kChunkSize);
                chunkBreaks_.push_back(0);
            }
            std::string& c = chunks_.back();
            const std::size_t n = std::min(kChunkSize - c.size(), text.size());
            const std::string_view part = text.substr(0, n);
            c.append(part);
            chunkBreaks_.back() += static_cast<std::size_t>(std::count(part.begin(), part.end(), '\n'));
            size_ += n;
            text.remove_prefix(n);
        }
    }

    // Zero-copy iteration over [offset, offset+len).
    template <class F>
    void forEachSegment(std::size_t offset, std::size_t len, F&& cb) const {
        assert(offset + len <= size_);
        std::size_t chunk = offset / kChunkSize, in = offset % kChunkSize;
        while (len > 0) {
            const std::string& c = chunks_[chunk];
            const std::size_t n = std::min(len, c.size() - in);
            cb(std::string_view(c).substr(in, n));
            len -= n;
            in = 0;
            ++chunk;
        }
    }

    std::string slice(std::size_t offset, std::size_t len) const {
        std::string out;
        out.reserve(len);
        forEachSegment(offset, len, [&](std::string_view s) { out.append(s); });
        return out;
    }

    std::size_t countNewlines(std::size_t offset, std::size_t len) const {
        std::size_t total = 0, chunk = offset / kChunkSize, in = offset % kChunkSize;
        while (len > 0) {
            const std::string& c = chunks_[chunk];
            const std::size_t n = std::min(len, c.size() - in);
            if (in == 0 && n == c.size())
                total += chunkBreaks_[chunk];
            else
                total += static_cast<std::size_t>(std::count(c.begin() + static_cast<std::ptrdiff_t>(in),
                                                             c.begin() + static_cast<std::ptrdiff_t>(in + n), '\n'));
            len -= n;
            in = 0;
            ++chunk;
        }
        return total;
    }

    // Index (relative to `offset`) of the n-th (1-based) '\n' within the range, or npos.
    std::size_t findNthNewline(std::size_t offset, std::size_t len, std::size_t n) const {
        std::size_t consumed = 0, chunk = offset / kChunkSize, in = offset % kChunkSize;
        while (len > 0) {
            const std::string& c = chunks_[chunk];
            const std::size_t seg = std::min(len, c.size() - in);
            const std::size_t cnt = (in == 0 && seg == c.size())
                                        ? chunkBreaks_[chunk]
                                        : static_cast<std::size_t>(std::count(
                                              c.begin() + static_cast<std::ptrdiff_t>(in),
                                              c.begin() + static_cast<std::ptrdiff_t>(in + seg), '\n'));
            if (cnt < n) {
                n -= cnt;
            } else {
                std::size_t pos = in;
                for (;;) {
                    pos = c.find('\n', pos);
                    if (--n == 0) return consumed + (pos - in);
                    ++pos;
                }
            }
            consumed += seg;
            len -= seg;
            in = 0;
            ++chunk;
        }
        return npos;
    }

private:
    std::vector<std::string> chunks_;
    std::vector<std::size_t> chunkBreaks_;
    std::size_t size_ = 0;
};

// Owns the two buffers a piece table refers to.
class BufferStore {
public:
    ChunkedBuffer& get(BufferId id) { return id == BufferId::Original ? original_ : add_; }
    const ChunkedBuffer& get(BufferId id) const { return id == BufferId::Original ? original_ : add_; }
    ChunkedBuffer& original() { return original_; }
    ChunkedBuffer& add() { return add_; }
    void clear() { original_ = ChunkedBuffer{}; add_ = ChunkedBuffer{}; }

private:
    ChunkedBuffer original_;
    ChunkedBuffer add_;
};

} // namespace ced
