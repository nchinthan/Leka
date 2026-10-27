#pragma once
#include "datasource/IDataSource.h"

#include <filesystem>
#include <fstream>

namespace ced {

// Local file. Writes go to "<path>.tmp~" and are renamed over the target on commit,
// so a failed save never damages the existing file.
class LocalFileDataSource final : public IDataSource {
public:
    explicit LocalFileDataSource(std::filesystem::path path) : path_(std::move(path)) {}
    ~LocalFileDataSource() override { abortWrite(); }

    std::string name() const override { return path_.filename().string(); }
    bool exists() const override;
    bool open(std::string& error) override;
    std::optional<std::uint64_t> size() const override;
    bool read(std::uint64_t offset, char* dst, std::size_t max, std::size_t& got, std::string& error) override;

    bool supportsWrite() const override { return true; }
    bool beginWrite(std::string& error) override;
    bool writeChunk(std::string_view data, std::string& error) override;
    bool commitWrite(std::string& error) override;
    void abortWrite() override;

private:
    std::filesystem::path tempPath() const;

    std::filesystem::path path_;
    std::ifstream in_;
    std::ofstream out_;
    bool writing_ = false;
};

} // namespace ced
