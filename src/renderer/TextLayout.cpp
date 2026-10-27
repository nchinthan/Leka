#include "renderer/TextLayout.h"

namespace ced {

namespace {
// Appends the cells for one byte; returns cells produced.
std::size_t cellsFor(char c, std::size_t at, std::string* out) {
    if (c == '\t') {
        const std::size_t n = kTabWidth - (at % kTabWidth);
        if (out) out->append(n, ' ');
        return n;
    }
    const auto u = static_cast<unsigned char>(c);
    if (out) out->push_back(u >= 0x20 && u < 0x7f ? c : '?');
    return 1;
}
} // namespace

std::string expandForDisplay(std::string_view line) {
    std::string out;
    out.reserve(line.size());
    for (char c : line) cellsFor(c, out.size(), &out);
    return out;
}

std::size_t displayColumn(std::string_view line, std::size_t col) {
    std::size_t cell = 0;
    for (std::size_t i = 0; i < col && i < line.size(); ++i) cell += cellsFor(line[i], cell, nullptr);
    return cell + (col > line.size() ? col - line.size() : 0);
}

} // namespace ced
