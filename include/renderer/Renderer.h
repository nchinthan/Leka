#pragma once
#include "document/IDocumentView.h"
#include "renderer/DirtyRegion.h"
#include "renderer/ITerminal.h"
#include "renderer/RenderedScreen.h"
#include "state/ViewState.h"

#include <memory>
#include <string>
#include <vector>

namespace ced {

struct StatusInfo {
    std::string modeName;
    std::string fileName;
    std::string message;
    std::string commandText;
    bool commandActive = false;
    bool modified = false;
};

// Everything a layer may read. The renderer never sees the piece table or any state class.
struct RenderContext {
    const IDocumentView& document;
    const ViewState& view;
    const StatusInfo& status;
    int width;
    int height;
    int cursorRow; // physical cursor, decided by the controller from logical state
    int cursorCol;
};

// A renderable layer owns some screen rows. New UI (line numbers, minimap, ...) = new layer.
class IRenderLayer {
public:
    virtual ~IRenderLayer() = default;
    virtual bool ownsRow(int row, const RenderContext& ctx) const = 0;
    virtual bool isRowDirty(int row, const DirtyRegion& region, const RenderContext& ctx) const = 0;
    virtual std::string renderRow(int row, const RenderContext& ctx) const = 0; // desired cells (any length)
};

class DocumentLayer final : public IRenderLayer {
public:
    bool ownsRow(int row, const RenderContext& ctx) const override { return row >= 0 && row < ctx.height - 1; }
    bool isRowDirty(int row, const DirtyRegion& region, const RenderContext& ctx) const override;
    std::string renderRow(int row, const RenderContext& ctx) const override;
};

class StatusBarLayer final : public IRenderLayer {
public:
    bool ownsRow(int row, const RenderContext& ctx) const override { return row == ctx.height - 1; }
    bool isRowDirty(int, const DirtyRegion&, const RenderContext&) const override { return true; } // one cheap row
    std::string renderRow(int row, const RenderContext& ctx) const override;
};

struct RenderStats {
    std::size_t fullScreenRedraws = 0;
    std::size_t writeCalls = 0;
    std::size_t clearCalls = 0;
    std::size_t cellsTouched = 0;
};

// Reconciles the desired screen with the last-rendered screen and emits only the differences.
class Renderer {
public:
    explicit Renderer(ITerminal& terminal) : terminal_(terminal) {}

    void addLayer(std::unique_ptr<IRenderLayer> layer) { layers_.push_back(std::move(layer)); }
    void invalidateAll() { needsFull_ = true; }
    void render(const RenderContext& ctx, DirtyRegion region);

    const RenderedScreen& screen() const { return screen_; }
    const RenderStats& stats() const { return stats_; }

private:
    void diffRow(int row, const std::string& desired);

    ITerminal& terminal_;
    std::vector<std::unique_ptr<IRenderLayer>> layers_;
    RenderedScreen screen_;
    RenderStats stats_;
    bool needsFull_ = true;
};

} // namespace ced
