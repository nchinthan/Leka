#include "document/TextProcessBackend.h"

#include <vector>

namespace ced {

TextProcessBackend::TextProcessBackend() : table_(buffers_) {}

bool TextProcessBackend::fail(const std::string& error) {
    lastError_ = error;
    status_ = DocumentStatus::Error;
    table_.reset();
    return false;
}

bool TextProcessBackend::open(std::unique_ptr<IDataSource> source, std::string& error) {
    source_ = std::move(source);
    status_ = DocumentStatus::Opening;
    buffers_.clear();
    table_.reset();
    history_.reset();
    lineEnding_ = LineEnding::LF;
    lastError_.clear();

    if (!source_->exists()) { // new file
        table_.initFromOriginal();
        status_ = DocumentStatus::Ready;
        return true;
    }
    if (!source_->open(error)) return fail(error);

    std::vector<char> raw(ChunkedBuffer::kChunkSize);
    std::string normalized;
    std::uint64_t offset = 0;
    std::size_t crlf = 0, lf = 0;
    bool pendingCR = false;

    for (;;) {
        std::size_t got = 0;
        if (!source_->read(offset, raw.data(), raw.size(), got, error)) return fail(error);
        if (got == 0) break;
        offset += got;
        normalized.clear();
        for (std::size_t i = 0; i < got; ++i) {
            const char c = raw[i];
            if (pendingCR) {
                pendingCR = false;
                if (c == '\n') { normalized.push_back('\n'); ++crlf; continue; }
                normalized.push_back('\r');
            }
            if (c == '\r') { pendingCR = true; continue; }
            if (c == '\n') ++lf;
            normalized.push_back(c);
        }
        buffers_.original().append(normalized);
    }
    if (pendingCR) buffers_.original().append("\r");

    lineEnding_ = crlf > lf ? LineEnding::CRLF : LineEnding::LF;
    table_.initFromOriginal();
    status_ = DocumentStatus::Ready;
    return true;
}

bool TextProcessBackend::save(std::string& error) {
    if (status_ != DocumentStatus::Ready) { error = "document is not ready to be saved"; return false; }
    if (!source_ || !source_->supportsWrite()) { error = "this data source is read-only"; return false; }

    status_ = DocumentStatus::Saving;
    bool ok = source_->beginWrite(error);
    if (ok) {
        std::string converted;
        table_.forEachSegment(0, table_.size(), [&](std::string_view seg) {
            if (!ok) return;
            if (lineEnding_ == LineEnding::LF) {
                ok = source_->writeChunk(seg, error);
                return;
            }
            converted.clear();
            for (char c : seg) {
                if (c == '\n') converted.push_back('\r');
                converted.push_back(c);
            }
            ok = source_->writeChunk(converted, error);
        });
        if (ok) ok = source_->commitWrite(error);
        if (!ok) source_->abortWrite();
    }
    status_ = DocumentStatus::Ready;
    if (ok) history_.markClean(); // a failed save must NOT mark the document saved
    return ok;
}

std::optional<ChangeInfo> TextProcessBackend::insert(std::size_t position, std::string_view text) {
    if (!canEdit() || position > table_.size() || text.empty()) return std::nullopt;
    auto op = std::make_unique<InsertOperation>(position, std::string(text));
    auto change = op->apply(table_);
    if (change) history_.record(std::move(op));
    return change;
}

std::optional<ChangeInfo> TextProcessBackend::erase(std::size_t position, std::size_t length) {
    if (!canEdit() || length == 0 || position > table_.size() || length > table_.size() - position)
        return std::nullopt;
    auto op = std::make_unique<DeleteOperation>(position, table_.read(position, length));
    auto change = op->apply(table_);
    if (change) history_.record(std::move(op));
    return change;
}

std::optional<ChangeInfo> TextProcessBackend::replace(std::size_t position, std::size_t length, std::string_view text) {
    if (!canEdit() || position > table_.size() || length > table_.size() - position) return std::nullopt;
    history_.beginGroup();
    std::optional<ChangeInfo> total;
    if (length > 0) total = erase(position, length);
    if (!text.empty()) {
        auto ins = insert(position, text);
        if (ins) total = total ? ChangeInfo::merge(*total, *ins) : *ins;
    }
    history_.endGroup();
    return total;
}

std::optional<HistoryOutcome> TextProcessBackend::undo() { return canEdit() ? history_.undo(table_) : std::nullopt; }
std::optional<HistoryOutcome> TextProcessBackend::redo() { return canEdit() ? history_.redo(table_) : std::nullopt; }

std::size_t TextProcessBackend::lineLength(std::size_t line) const {
    if (line >= lineCount()) return 0;
    const std::size_t start = table_.lineStart(line);
    const std::size_t end = line + 1 < lineCount() ? table_.lineStart(line + 1) - 1 : table_.size();
    return end - start;
}

std::string TextProcessBackend::lineText(std::size_t line) const {
    if (line >= lineCount()) return {};
    return table_.read(table_.lineStart(line), lineLength(line));
}

} // namespace ced
