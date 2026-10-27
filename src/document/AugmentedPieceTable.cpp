#include "document/AugmentedPieceTable.h"

#include <algorithm>
#include <utility>

namespace ced {
namespace detail {
struct PieceNode {
    Piece piece{};
    std::uint32_t priority = 0;
    std::size_t subLen = 0;
    std::size_t subBreaks = 0;
    std::unique_ptr<PieceNode> left;
    std::unique_ptr<PieceNode> right;
};
} // namespace detail

namespace {
using detail::PieceNode;
using NodePtr = std::unique_ptr<PieceNode>;

std::size_t lenOf(const NodePtr& n) { return n ? n->subLen : 0; }
std::size_t brOf(const NodePtr& n) { return n ? n->subBreaks : 0; }

void pull(PieceNode& n) {
    n.subLen = lenOf(n.left) + n.piece.length + lenOf(n.right);
    n.subBreaks = brOf(n.left) + n.piece.lineBreaks + brOf(n.right);
}

NodePtr makeNode(const Piece& p, std::uint32_t priority) {
    auto n = std::make_unique<PieceNode>();
    n->piece = p;
    n->priority = priority;
    pull(*n);
    return n;
}

NodePtr merge(NodePtr a, NodePtr b) {
    if (!a) return b;
    if (!b) return a;
    if (a->priority >= b->priority) {
        a->right = merge(std::move(a->right), std::move(b));
        pull(*a);
        return a;
    }
    b->left = merge(std::move(a), std::move(b->left));
    pull(*b);
    return b;
}

// Splits so the left tree holds exactly `pos` characters. A piece straddling `pos` is split in two.
std::pair<NodePtr, NodePtr> split(NodePtr root, std::size_t pos, const BufferStore& bs) {
    if (!root) return {nullptr, nullptr};
    const std::size_t leftLen = lenOf(root->left);
    if (pos <= leftLen) {
        auto [a, b] = split(std::move(root->left), pos, bs);
        root->left = std::move(b);
        pull(*root);
        return {std::move(a), std::move(root)};
    }
    const std::size_t pieceEnd = leftLen + root->piece.length;
    if (pos >= pieceEnd) {
        auto [a, b] = split(std::move(root->right), pos - pieceEnd, bs);
        root->right = std::move(a);
        pull(*root);
        return {std::move(root), std::move(b)};
    }
    // Strictly inside the piece: 0 < k < length, so neither half is empty.
    const std::size_t k = pos - leftLen;
    Piece& p = root->piece;
    const std::size_t leftBreaks = bs.get(p.buffer).countNewlines(p.offset, k);
    const Piece rp{p.buffer, p.offset + k, p.length - k, p.lineBreaks - leftBreaks};
    NodePtr rnode = makeNode(rp, root->priority); // same priority keeps heap order
    rnode->right = std::move(root->right);
    root->right = nullptr;
    p.length = k;
    p.lineBreaks = leftBreaks;
    pull(*rnode);
    pull(*root);
    return {std::move(root), std::move(rnode)};
}

PieceNode* rightmost(PieceNode* n) {
    while (n && n->right) n = n->right.get();
    return n;
}
PieceNode* leftmost(PieceNode* n) {
    while (n && n->left) n = n->left.get();
    return n;
}

void extendRightmost(PieceNode& n, std::size_t addLen, std::size_t addBreaks) {
    if (n.right) {
        extendRightmost(*n.right, addLen, addBreaks);
    } else {
        n.piece.length += addLen;
        n.piece.lineBreaks += addBreaks;
    }
    pull(n);
}

Piece popLeftmost(NodePtr& n) {
    if (n->left) {
        Piece p = popLeftmost(n->left);
        pull(*n);
        return p;
    }
    Piece p = n->piece;
    n = std::move(n->right);
    return p;
}

// If the last piece of `l` and first piece of `r` are contiguous in the same buffer, fuse them.
void tryCoalesce(NodePtr& l, NodePtr& r) {
    PieceNode* a = rightmost(l.get());
    PieceNode* b = leftmost(r.get());
    if (a && b && a->piece.buffer == b->piece.buffer && a->piece.offset + a->piece.length == b->piece.offset) {
        const Piece bp = popLeftmost(r);
        extendRightmost(*l, bp.length, bp.lineBreaks);
    }
}

void walk(const PieceNode* n, std::size_t base, std::size_t lo, std::size_t hi, const BufferStore& bs,
          const std::function<void(std::string_view)>& cb) {
    if (!n || lo >= hi) return;
    if (hi <= base || lo >= base + n->subLen) return;
    const std::size_t leftLen = lenOf(n->left);
    walk(n->left.get(), base, lo, hi, bs, cb);
    const std::size_t ps = base + leftLen, pe = ps + n->piece.length;
    if (lo < pe && hi > ps) {
        const std::size_t s = std::max(lo, ps), e = std::min(hi, pe);
        bs.get(n->piece.buffer).forEachSegment(n->piece.offset + (s - ps), e - s, cb);
    }
    walk(n->right.get(), pe, lo, hi, bs, cb);
}

std::size_t countPieces(const PieceNode* n) {
    return n ? 1 + countPieces(n->left.get()) + countPieces(n->right.get()) : 0;
}

bool check(const PieceNode* n, const BufferStore& bs) {
    if (!n) return true;
    if (n->piece.length == 0) return false;
    if (n->piece.offset + n->piece.length > bs.get(n->piece.buffer).size()) return false;
    if (n->piece.lineBreaks != bs.get(n->piece.buffer).countNewlines(n->piece.offset, n->piece.length)) return false;
    if (n->subLen != lenOf(n->left) + n->piece.length + lenOf(n->right)) return false;
    if (n->subBreaks != brOf(n->left) + n->piece.lineBreaks + brOf(n->right)) return false;
    if (n->left && n->left->priority > n->priority) return false;
    if (n->right && n->right->priority > n->priority) return false;
    return check(n->left.get(), bs) && check(n->right.get(), bs);
}
} // namespace

AugmentedPieceTable::AugmentedPieceTable(BufferStore& buffers) : buffers_(buffers) {}
AugmentedPieceTable::~AugmentedPieceTable() = default;

std::uint32_t AugmentedPieceTable::nextPriority() {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return rng_;
}

void AugmentedPieceTable::reset() { root_.reset(); }

void AugmentedPieceTable::initFromOriginal() {
    root_.reset();
    const ChunkedBuffer& orig = buffers_.original();
    if (orig.size() > 0)
        root_ = makeNode(Piece{BufferId::Original, 0, orig.size(), orig.countNewlines(0, orig.size())},
                         nextPriority());
}

std::size_t AugmentedPieceTable::size() const { return lenOf(root_); }
std::size_t AugmentedPieceTable::lineBreakCount() const { return brOf(root_); }
std::size_t AugmentedPieceTable::pieceCount() const { return countPieces(root_.get()); }

bool AugmentedPieceTable::insert(std::size_t position, std::string_view text) {
    if (position > size()) return false;
    if (text.empty()) return true;

    ChunkedBuffer& add = buffers_.add();
    const std::size_t off = add.size();
    const std::size_t breaks = static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n'));
    add.append(text);

    auto [l, r] = split(std::move(root_), position, buffers_);
    PieceNode* last = rightmost(l.get());
    if (last && last->piece.buffer == BufferId::Add && last->piece.offset + last->piece.length == off) {
        extendRightmost(*l, text.size(), breaks); // typing run: grow the previous piece
    } else {
        l = merge(std::move(l), makeNode(Piece{BufferId::Add, off, text.size(), breaks}, nextPriority()));
    }
    root_ = merge(std::move(l), std::move(r));
    return true;
}

bool AugmentedPieceTable::erase(std::size_t position, std::size_t length) {
    if (position > size() || length > size() - position) return false;
    if (length == 0) return true;

    auto [left, rest] = split(std::move(root_), position, buffers_);
    auto [mid, right] = split(std::move(rest), length, buffers_);
    mid.reset(); // buffers are untouched; the text is simply no longer referenced
    tryCoalesce(left, right);
    root_ = merge(std::move(left), std::move(right));
    return true;
}

void AugmentedPieceTable::forEachSegment(std::size_t position, std::size_t length,
                                         const std::function<void(std::string_view)>& cb) const {
    const std::size_t total = size();
    if (position >= total) return;
    const std::size_t hi = std::min(total, position + std::min(length, total - position));
    walk(root_.get(), 0, position, hi, buffers_, cb);
}

std::string AugmentedPieceTable::read(std::size_t position, std::size_t length) const {
    std::string out;
    out.reserve(std::min(length, size()));
    forEachSegment(position, length, [&](std::string_view s) { out.append(s); });
    return out;
}

std::size_t AugmentedPieceTable::lineStart(std::size_t line) const {
    if (line == 0) return 0;
    if (line > lineBreakCount()) return size();
    std::size_t k = line, base = 0;
    const PieceNode* n = root_.get();
    while (n) {
        const std::size_t leftBreaks = brOf(n->left);
        if (k <= leftBreaks) {
            n = n->left.get();
            continue;
        }
        k -= leftBreaks;
        base += lenOf(n->left);
        if (k <= n->piece.lineBreaks) {
            const std::size_t idx =
                buffers_.get(n->piece.buffer).findNthNewline(n->piece.offset, n->piece.length, k);
            return base + idx + 1;
        }
        k -= n->piece.lineBreaks;
        base += n->piece.length;
        n = n->right.get();
    }
    return size(); // unreachable for consistent aggregates
}

std::size_t AugmentedPieceTable::lineOfOffset(std::size_t offset) const {
    offset = std::min(offset, size());
    std::size_t count = 0;
    const PieceNode* n = root_.get();
    while (n) {
        const std::size_t leftLen = lenOf(n->left);
        if (offset <= leftLen) {
            n = n->left.get();
            continue;
        }
        count += brOf(n->left);
        offset -= leftLen;
        if (offset <= n->piece.length) return count + buffers_.get(n->piece.buffer).countNewlines(n->piece.offset, offset);
        count += n->piece.lineBreaks;
        offset -= n->piece.length;
        n = n->right.get();
    }
    return count;
}

bool AugmentedPieceTable::validate() const { return check(root_.get(), buffers_); }

} // namespace ced
