#pragma once
#include "document/BufferStore.h"
#include "document/DocumentOperation.h"
#include "document/Piece.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace ced {

namespace detail {
struct PieceNode; // treap node: piece + subtree aggregates (length, line breaks)
}

// Piece table whose pieces live in an implicit treap (a balanced sequence tree).
// Every node caches its subtree's total length and '\n' count, so offset lookup,
// line lookup, insert and erase are O(log P) in the piece count (plus the cost of
// scanning inside one piece, which is bounded by per-chunk cached counts).
//
// Never depends on controller, renderer or data sources. Buffers are owned elsewhere.
class AugmentedPieceTable final : public IEditTarget {
public:
    explicit AugmentedPieceTable(BufferStore& buffers);
    ~AugmentedPieceTable() override;
    AugmentedPieceTable(const AugmentedPieceTable&) = delete;
    AugmentedPieceTable& operator=(const AugmentedPieceTable&) = delete;

    void reset();               // empty sequence
    void initFromOriginal();    // one piece spanning the whole original buffer

    std::size_t size() const;
    std::size_t lineBreakCount() const;
    std::size_t lineCount() const { return lineBreakCount() + 1; }
    std::size_t pieceCount() const;

    bool insert(std::size_t position, std::string_view text) override;
    bool erase(std::size_t position, std::size_t length) override;

    std::string read(std::size_t position, std::size_t length) const;
    void forEachSegment(std::size_t position, std::size_t length,
                        const std::function<void(std::string_view)>& cb) const;

    // Offset of the first character of `line`. Lines past the end clamp to size().
    std::size_t lineStart(std::size_t line) const;
    // Zero-based line containing `offset` (offset is clamped to size()).
    std::size_t lineOfOffset(std::size_t offset) const;

    // Checks aggregates, heap order and "no empty pieces". For tests.
    bool validate() const;

private:
    std::uint32_t nextPriority();

    BufferStore& buffers_;
    std::unique_ptr<detail::PieceNode> root_;
    std::uint32_t rng_ = 2463534242u;
};

} // namespace ced
