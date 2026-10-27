#pragma once
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ced {

// Minimal editing surface that operations replay against. Implemented by the piece table.
class IEditTarget {
public:
    virtual ~IEditTarget() = default;
    virtual bool insert(std::size_t position, std::string_view text) = 0;
    virtual bool erase(std::size_t position, std::size_t length) = 0;
};

// Describes what an edit touched, so callers can compute rendering invalidation
// without knowing anything about the piece table.
struct ChangeInfo {
    std::size_t offset = 0;
    std::size_t removed = 0;
    std::size_t inserted = 0;
    bool lineBreaksTouched = false; // true => lines after `offset` may have shifted

    static ChangeInfo merge(const ChangeInfo& a, const ChangeInfo& b) {
        return {std::min(a.offset, b.offset), a.removed + b.removed, a.inserted + b.inserted,
                a.lineBreaksTouched || b.lineBreaksTouched};
    }
};

// A logical edit that can be applied and reversed. Operations own their data
// (no references to buffers), so they never dangle.
class DocumentOperation {
public:
    virtual ~DocumentOperation() = default;
    virtual std::optional<ChangeInfo> apply(IEditTarget& target) = 0;
    virtual std::optional<ChangeInfo> revert(IEditTarget& target) = 0;
    virtual std::size_t cursorAfterApply() const = 0;
    virtual std::size_t cursorAfterRevert() const = 0;
    virtual bool empty() const { return false; }
};

class InsertOperation final : public DocumentOperation {
public:
    InsertOperation(std::size_t pos, std::string text) : pos_(pos), text_(std::move(text)) {}
    std::optional<ChangeInfo> apply(IEditTarget& t) override {
        if (!t.insert(pos_, text_)) return std::nullopt;
        return ChangeInfo{pos_, 0, text_.size(), hasBreak()};
    }
    std::optional<ChangeInfo> revert(IEditTarget& t) override {
        if (!t.erase(pos_, text_.size())) return std::nullopt;
        return ChangeInfo{pos_, text_.size(), 0, hasBreak()};
    }
    std::size_t cursorAfterApply() const override { return pos_ + text_.size(); }
    std::size_t cursorAfterRevert() const override { return pos_; }

private:
    bool hasBreak() const { return text_.find('\n') != std::string::npos; }
    std::size_t pos_;
    std::string text_;
};

class DeleteOperation final : public DocumentOperation {
public:
    DeleteOperation(std::size_t pos, std::string text) : pos_(pos), text_(std::move(text)) {}
    std::optional<ChangeInfo> apply(IEditTarget& t) override {
        if (!t.erase(pos_, text_.size())) return std::nullopt;
        return ChangeInfo{pos_, text_.size(), 0, hasBreak()};
    }
    std::optional<ChangeInfo> revert(IEditTarget& t) override {
        if (!t.insert(pos_, text_)) return std::nullopt;
        return ChangeInfo{pos_, 0, text_.size(), hasBreak()};
    }
    std::size_t cursorAfterApply() const override { return pos_; }
    std::size_t cursorAfterRevert() const override { return pos_ + text_.size(); }

private:
    bool hasBreak() const { return text_.find('\n') != std::string::npos; }
    std::size_t pos_;
    std::string text_;
};

// Several operations treated as one undo step. Apply is all-or-nothing.
class CompositeOperation final : public DocumentOperation {
public:
    void add(std::unique_ptr<DocumentOperation> op) { ops_.push_back(std::move(op)); }
    bool empty() const override { return ops_.empty(); }

    std::optional<ChangeInfo> apply(IEditTarget& t) override {
        std::optional<ChangeInfo> total;
        for (std::size_t i = 0; i < ops_.size(); ++i) {
            auto r = ops_[i]->apply(t);
            if (!r) {
                for (std::size_t j = i; j-- > 0;) ops_[j]->revert(t); // roll back
                return std::nullopt;
            }
            total = total ? ChangeInfo::merge(*total, *r) : *r;
        }
        return finish(total);
    }
    std::optional<ChangeInfo> revert(IEditTarget& t) override {
        std::optional<ChangeInfo> total;
        for (std::size_t i = ops_.size(); i-- > 0;) {
            auto r = ops_[i]->revert(t);
            if (!r) return std::nullopt; // cannot happen for a consistent history
            total = total ? ChangeInfo::merge(*total, *r) : *r;
        }
        return finish(total);
    }
    std::size_t cursorAfterApply() const override { return ops_.empty() ? 0 : ops_.back()->cursorAfterApply(); }
    std::size_t cursorAfterRevert() const override { return ops_.empty() ? 0 : ops_.front()->cursorAfterRevert(); }

private:
    // Conservative: multi-step edits may shift later lines. The renderer diff keeps output minimal anyway.
    std::optional<ChangeInfo> finish(std::optional<ChangeInfo> total) const {
        if (total && ops_.size() > 1) total->lineBreaksTouched = true;
        return total;
    }
    std::vector<std::unique_ptr<DocumentOperation>> ops_;
};

} // namespace ced
