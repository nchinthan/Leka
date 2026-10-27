#pragma once
#include <cstddef>
#include <cstdint>

namespace ced {

enum class BufferId : std::uint8_t { Original, Add };

// A piece references a contiguous range of one buffer. `lineBreaks` is the
// number of '\n' characters inside the referenced range (cached metadata).
struct Piece {
    BufferId buffer;
    std::size_t offset;
    std::size_t length;
    std::size_t lineBreaks;
};

} // namespace ced
