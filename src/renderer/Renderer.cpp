#include "renderer/Renderer.h"

#include "renderer/TextLayout.h"

#include <algorithm>

namespace ced {

bool DocumentLayer::isRowDirty(int row, const DirtyRegion& region, const RenderContext& ctx) const {
    return region.coversLine(ctx.view.firstVisibleLine + static_cast<std::size_t>(row));
}

std::string DocumentLayer::renderRow(int row, const RenderContext& ctx) const {
    const std::size_t line = ctx.view.firstVisibleLine + static_cast<std::size_t>(row);
    if (line >= ctx.document.lineCount()) return {};
    const std::string cells = expandForDisplay(ctx.document.lineText(line));
    if (ctx.view.horizontalOffset >= cells.size()) return {};
    return cells.substr(ctx.view.horizontalOffset);
}

std::string StatusBarLayer::renderRow(int, const RenderContext& ctx) const {
    const std::size_t w = static_cast<std::size_t>(std::max(ctx.width, 0));
    const StatusInfo& s = ctx.status;
    if (s.commandActive) return ":" + s.commandText;

    std::string left = " " + s.modeName + " | " + s.fileName + (s.modified ? " [+]" : "");
    if (!s.message.empty()) left += " | " + s.message;
    const std::string right =
        "Ln " + std::to_string(ctx.view.cursorLine + 1) + ", Col " + std::to_string(ctx.view.cursorCol + 1) + " ";
    if (left.size() + right.size() >= w) return left;
    return left + std::string(w - left.size() - right.size(), ' ') + right;
}

void Renderer::diffRow(int row, const std::string& desiredIn) {
    std::string& current = screen_.row(row);
    const std::size_t w = current.size();
    std::string desired = desiredIn.substr(0, w);
    desired.resize(w, ' ');

    std::size_t c = 0;
    while (c < w) {
        if (current[c] == desired[c]) { ++c; continue; }
        const std::size_t start = c;
        while (c < w && current[c] != desired[c]) ++c;
        const std::string run = desired.substr(start, c - start);
        if (run.find_first_not_of(' ') == std::string::npos) {
            terminal_.clearRange(row, static_cast<int>(start), static_cast<int>(run.size()));
            ++stats_.clearCalls;
        } else {
            terminal_.write(row, static_cast<int>(start), run);
            ++stats_.writeCalls;
        }
        stats_.cellsTouched += run.size();
        current.replace(start, run.size(), run);
    }
}

void Renderer::render(const RenderContext& ctx, DirtyRegion region) {
    if (needsFull_ || screen_.width() != ctx.width || screen_.height() != ctx.height) {
        region = DirtyRegion::fullScreen();
        needsFull_ = false;
    }
    if (region.isFullScreen()) {
        terminal_.clearScreen();
        screen_.reset(ctx.width, ctx.height);
        ++stats_.fullScreenRedraws;
    }
    for (int row = 0; row < ctx.height; ++row) {
        for (const auto& layer : layers_) {
            if (!layer->ownsRow(row, ctx)) continue;
            if (region.isFullScreen() || layer->isRowDirty(row, region, ctx)) diffRow(row, layer->renderRow(row, ctx));
            break;
        }
    }
    terminal_.setCursor(ctx.cursorRow, ctx.cursorCol);
    terminal_.flush();
}

} // namespace ced
