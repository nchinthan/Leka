#include "document/HistoryManager.h"

namespace ced {

void HistoryManager::push(std::unique_ptr<DocumentOperation> op) {
    // If the saved state lived in the redo branch we are about to discard, it is unreachable now.
    if (cleanIndex_ && *cleanIndex_ > undo_.size()) cleanIndex_.reset();
    redo_.clear();
    undo_.push_back(std::move(op));
}

void HistoryManager::record(std::unique_ptr<DocumentOperation> op) {
    if (groupOpen())
        group_->add(std::move(op));
    else
        push(std::move(op));
}

void HistoryManager::beginGroup() {
    if (groupDepth_++ == 0) group_ = std::make_unique<CompositeOperation>();
}

void HistoryManager::endGroup() {
    if (groupDepth_ == 0) return;
    if (--groupDepth_ == 0) {
        if (!group_->empty()) push(std::move(group_));
        group_.reset();
    }
}

std::optional<HistoryOutcome> HistoryManager::undo(IEditTarget& target) {
    if (!canUndo()) return std::nullopt;
    auto op = std::move(undo_.back());
    undo_.pop_back();
    auto change = op->revert(target); // does NOT go through record(): undo never logs itself
    if (!change) {
        undo_.push_back(std::move(op));
        return std::nullopt;
    }
    const std::size_t cursor = op->cursorAfterRevert();
    redo_.push_back(std::move(op));
    return HistoryOutcome{*change, cursor};
}

std::optional<HistoryOutcome> HistoryManager::redo(IEditTarget& target) {
    if (!canRedo()) return std::nullopt;
    auto op = std::move(redo_.back());
    redo_.pop_back();
    auto change = op->apply(target);
    if (!change) {
        redo_.push_back(std::move(op));
        return std::nullopt;
    }
    const std::size_t cursor = op->cursorAfterApply();
    undo_.push_back(std::move(op));
    return HistoryOutcome{*change, cursor};
}

void HistoryManager::reset() {
    undo_.clear();
    redo_.clear();
    group_.reset();
    groupDepth_ = 0;
    cleanIndex_ = 0;
}

} // namespace ced
