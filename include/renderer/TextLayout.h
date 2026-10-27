#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace ced {

// Display model: one byte -> one terminal cell. Tabs expand to the next multiple of
// kTabWidth; control characters and bytes >= 0x80 are shown as '?'.
// (Limitation: no wide/UTF-8 aware layout in this version.)
inline constexpr std::size_t kTabWidth = 4;

std::string expandForDisplay(std::string_view line);
std::size_t displayColumn(std::string_view line, std::size_t col); // cell index of character `col`

} // namespace ced
