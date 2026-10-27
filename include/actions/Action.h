#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace ced {

using ActionId = std::string;
using ModeId = std::string;

// Well-known action names. Adding a new action = adding a new name (anywhere) and a
// handler that understands it; no central enum or switch has to change.
namespace actions {
inline constexpr std::string_view InsertCharacter = "InsertCharacter";
inline constexpr std::string_view InsertNewline = "InsertNewline";
inline constexpr std::string_view DeleteBackward = "DeleteBackward";
inline constexpr std::string_view DeleteForward = "DeleteForward";
inline constexpr std::string_view MoveCursorLeft = "MoveCursorLeft";
inline constexpr std::string_view MoveCursorRight = "MoveCursorRight";
inline constexpr std::string_view MoveCursorUp = "MoveCursorUp";
inline constexpr std::string_view MoveCursorDown = "MoveCursorDown";
inline constexpr std::string_view MoveLineStart = "MoveLineStart";
inline constexpr std::string_view MoveLineEnd = "MoveLineEnd";
inline constexpr std::string_view SwitchMode = "SwitchMode";
inline constexpr std::string_view Undo = "Undo";
inline constexpr std::string_view Redo = "Redo";
inline constexpr std::string_view SaveDocument = "SaveDocument";
inline constexpr std::string_view ScrollViewport = "ScrollViewport";
} // namespace actions

namespace modes {
inline constexpr std::string_view Insert = "insert";
inline constexpr std::string_view Normal = "normal";
inline constexpr std::string_view Command = "command";
inline constexpr std::string_view Global = "*"; // key-binding scope that applies in every mode
} // namespace modes

// A semantic, mode-independent request. Carries only the parameters it needs.
struct Action {
    ActionId id;
    std::variant<std::monostate, std::string, long long> payload;

    static Action simple(std::string_view id) { return {std::string(id), std::monostate{}}; }
    static Action withText(std::string_view id, std::string text) { return {std::string(id), std::move(text)}; }
    static Action withNumber(std::string_view id, long long n) { return {std::string(id), n}; }
    static Action switchMode(std::string_view mode) { return withText(actions::SwitchMode, std::string(mode)); }

    const std::string* text() const { return std::get_if<std::string>(&payload); }
    long long number(long long fallback = 0) const {
        const auto* n = std::get_if<long long>(&payload);
        return n ? *n : fallback;
    }
    bool is(std::string_view name) const { return id == name; }
};

} // namespace ced
