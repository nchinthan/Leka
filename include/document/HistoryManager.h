#pragma once
#include "document/DocumentOperation.h"

#include <memory>
#include <optional>
#include <vector>

namespace ced {

struct HistoryOutcome {
    ChangeInfo change;
    std::size_t cursorOffset; // where the cursor should go (history does not own cursor state)
};

// Operation-log undo/redo. Stores operations, never document snapshots.
class HistoryManager {
public:
    // Record an operation that has ALREADY been applied successfully.
    void record(std::unique_ptr<DocumentOperation> op);

    void beginGroup();
    void endGroup();
    bool groupOpen() const { return groupDepth_ > 0; }

    std::optional<HistoryOutcome> undo(IEditTarget& target);
    std::optional<HistoryOutcome> redo(IEditTarget& target);

    bool canUndo() const { return !undo_.empty() && !groupOpen(); }
    bool canRedo() const { return !redo_.empty() && !groupOpen(); }
    std::size_t undoDepth() const { return undo_.size(); }
    std::size_t redoDepth() const { return redo_.size(); }

    void markClean() { cleanIndex_ = undo_.size(); }
    bool isClean() const { return cleanIndex_ && *cleanIndex_ == undo_.size(); }
    void reset();

private:
    void push(std::unique_ptr<DocumentOperation> op);

    std::vector<std::unique_ptr<DocumentOperation>> undo_;
    std::vector<std::unique_ptr<DocumentOperation>> redo_;
    std::unique_ptr<CompositeOperation> group_;
    int groupDepth_ = 0;
    std::optional<std::size_t> cleanIndex_ = 0; // undo depth at which the document equals the saved state
};

} // namespace ced
