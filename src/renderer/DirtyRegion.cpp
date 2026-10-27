#include "renderer/DirtyRegion.h"

#include <algorithm>

namespace ced {

DirtyRegion DirtyRegion::dirtyRange(std::size_t line, std::size_t startCol, std::size_t endCol) {
    DirtyRegion r;
    r.spans_.push_back({line, line, true, startCol, endCol});
    return r;
}

DirtyRegion DirtyRegion::dirtyLines(std::size_t firstLine, std::size_t lastLine) {
    DirtyRegion r;
    r.spans_.push_back({firstLine, std::max(firstLine, lastLine), false, 0, 0});
    return r;
}

DirtyRegion DirtyRegion::dirtyFromLine(std::size_t line) {
    DirtyRegion r;
    r.fromLine_ = line;
    return r;
}

DirtyRegion DirtyRegion::fullViewport() {
    DirtyRegion r;
    r.fullViewport_ = true;
    return r;
}

DirtyRegion DirtyRegion::fullScreen() {
    DirtyRegion r;
    r.fullScreen_ = true;
    return r;
}

void DirtyRegion::merge(const DirtyRegion& o) {
    fullScreen_ = fullScreen_ || o.fullScreen_;
    fullViewport_ = fullViewport_ || o.fullViewport_;
    if (o.fromLine_) fromLine_ = fromLine_ ? std::min(*fromLine_, *o.fromLine_) : *o.fromLine_;
    spans_.insert(spans_.end(), o.spans_.begin(), o.spans_.end());
}

bool DirtyRegion::coversLine(std::size_t line) const {
    if (fullScreen_ || fullViewport_) return true;
    if (fromLine_ && line >= *fromLine_) return true;
    return std::any_of(spans_.begin(), spans_.end(), [&](const Span& s) { return line >= s.first && line <= s.last; });
}

DirtyRegion::Kind DirtyRegion::kind() const {
    if (fullScreen_) return Kind::FullScreen;
    if (fullViewport_) return Kind::FullViewport;
    if (fromLine_) return Kind::DirtyFromLine;
    if (std::any_of(spans_.begin(), spans_.end(), [](const Span& s) { return !s.columnRange; })) return Kind::DirtyLine;
    if (!spans_.empty()) return Kind::DirtyRange;
    return Kind::Clean;
}

} // namespace ced
