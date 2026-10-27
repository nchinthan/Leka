#pragma once
#include "datasource/IDataSource.h"
#include "document/AugmentedPieceTable.h"
#include "document/BufferStore.h"
#include "document/HistoryManager.h"
#include "document/IDocumentView.h"

#include <memory>
#include <optional>
#include <string>

namespace ced {

enum class DocumentStatus { Unopened, Opening, PartiallyLoaded, Ready, Saving, Error };
enum class LineEnding { LF, CRLF };

// Document layer: owns buffers, piece table, history and the data source.
// Line-ending policy: CRLF is normalized to LF on load; the detected style
// (CRLF if CRLF lines outnumber bare LF lines) is re-applied on save.
// No rendering or platform code lives here.
class TextProcessBackend final : public IDocumentView {
public:
    TextProcessBackend();

    // Loads the source in chunks. A source that does not exist yields a new empty document.
    bool open(std::unique_ptr<IDataSource> source, std::string& error);
    bool save(std::string& error);

    // Edits. Return nullopt when rejected (not ready / out of range). Successful edits are logged.
    std::optional<ChangeInfo> insert(std::size_t position, std::string_view text);
    std::optional<ChangeInfo> erase(std::size_t position, std::size_t length);
    std::optional<ChangeInfo> replace(std::size_t position, std::size_t length, std::string_view text);

    std::optional<HistoryOutcome> undo();
    std::optional<HistoryOutcome> redo();
    void beginGroup() { history_.beginGroup(); }
    void endGroup() { history_.endGroup(); }
    bool canUndo() const { return history_.canUndo(); }
    bool canRedo() const { return history_.canRedo(); }

    bool isModified() const { return !history_.isClean(); }
    DocumentStatus status() const { return status_; }
    bool canEdit() const { return status_ == DocumentStatus::Ready; }
    const std::string& lastError() const { return lastError_; }
    LineEnding lineEnding() const { return lineEnding_; }
    std::string sourceName() const { return source_ ? source_->name() : std::string("[no name]"); }
    const AugmentedPieceTable& table() const { return table_; } // read-only, for diagnostics/tests

    // IDocumentView
    std::size_t size() const override { return table_.size(); }
    std::size_t lineCount() const override { return table_.lineCount(); }
    std::size_t lineStartOffset(std::size_t line) const override { return table_.lineStart(line); }
    std::size_t lineOfOffset(std::size_t offset) const override { return table_.lineOfOffset(offset); }
    std::size_t lineLength(std::size_t line) const override;
    std::string lineText(std::size_t line) const override;
    std::string readRange(std::size_t position, std::size_t length) const override { return table_.read(position, length); }

private:
    bool fail(const std::string& error);

    BufferStore buffers_;            // declared before table_ (table_ references it)
    AugmentedPieceTable table_;
    HistoryManager history_;
    std::unique_ptr<IDataSource> source_;
    DocumentStatus status_ = DocumentStatus::Unopened;
    LineEnding lineEnding_ = LineEnding::LF;
    std::string lastError_;
};

} // namespace ced
