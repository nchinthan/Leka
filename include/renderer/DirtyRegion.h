#pragma once
#include <cstddef>
#include <optional>
#include <vector>

namespace ced {

// What an action invalidated, in document-line terms. Combinable with merge().
// The renderer also diffs against what is already on screen, so a coarse region
// costs CPU but never extra terminal output.
class DirtyRegion {
public:
    enum class Kind { Clean, DirtyRange, DirtyLine, DirtyFromLine, FullViewport, FullScreen };

    static DirtyRegion clean() { return {}; }
    static DirtyRegion dirtyRange(std::size_t line, std::size_t startCol, std::size_t endCol);
    static DirtyRegion dirtyLines(std::size_t firstLine, std::size_t lastLine);
    static DirtyRegion dirtyFromLine(std::size_t line);
    static DirtyRegion fullViewport();
    static DirtyRegion fullScreen();

    void merge(const DirtyRegion& other);

    bool isClean() const { return kind() == Kind::Clean; }
    bool isFullScreen() const { return fullScreen_; }
    bool isFullViewport() const { return fullViewport_; }
    bool coversLine(std::size_t line) const;
    Kind kind() const; // strongest kind contained

private:
    struct Span {
        std::size_t first, last;
        bool columnRange;
        std::size_t startCol, endCol;
    };
    bool fullScreen_ = false;
    bool fullViewport_ = false;
    std::optional<std::size_t> fromLine_;
    std::vector<Span> spans_;
};

} // namespace ced
