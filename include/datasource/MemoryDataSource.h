#pragma once
#include "datasource/IDataSource.h"

#include <algorithm>
#include <cstring>

namespace ced {

// In-memory source; handy for tests and scratch documents.
class MemoryDataSource final : public IDataSource {
public:
    explicit MemoryDataSource(std::string content = {}, std::string name = "[memory]", bool exists = true)
        : content_(std::move(content)), name_(std::move(name)), exists_(exists) {}

    const std::string& content() const { return content_; }
    void setWriteFailure(bool fail) { failWrites_ = fail; }

    std::string name() const override { return name_; }
    bool exists() const override { return exists_; }
    bool open(std::string&) override { return true; }
    std::optional<std::uint64_t> size() const override { return content_.size(); }
    bool read(std::uint64_t offset, char* dst, std::size_t max, std::size_t& got, std::string&) override {
        if (offset >= content_.size()) { got = 0; return true; }
        got = std::min<std::size_t>(max, content_.size() - static_cast<std::size_t>(offset));
        std::memcpy(dst, content_.data() + offset, got);
        return true;
    }

    bool supportsWrite() const override { return true; }
    bool beginWrite(std::string& error) override {
        if (failWrites_) { error = "simulated write failure"; return false; }
        pending_.clear();
        return true;
    }
    bool writeChunk(std::string_view data, std::string&) override { pending_.append(data); return true; }
    bool commitWrite(std::string&) override { content_ = std::move(pending_); pending_.clear(); exists_ = true; return true; }
    void abortWrite() override { pending_.clear(); }

private:
    std::string content_, name_, pending_;
    bool exists_;
    bool failWrites_ = false;
};

} // namespace ced
