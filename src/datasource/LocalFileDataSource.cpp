#include "datasource/LocalFileDataSource.h"

namespace ced {

namespace fs = std::filesystem;

bool LocalFileDataSource::exists() const {
    std::error_code ec;
    return fs::is_regular_file(path_, ec);
}

bool LocalFileDataSource::open(std::string& error) {
    in_.open(path_, std::ios::binary);
    if (!in_) {
        error = "cannot open '" + path_.string() + "' for reading";
        return false;
    }
    return true;
}

std::optional<std::uint64_t> LocalFileDataSource::size() const {
    std::error_code ec;
    const auto s = fs::file_size(path_, ec);
    if (ec) return std::nullopt;
    return static_cast<std::uint64_t>(s);
}

bool LocalFileDataSource::read(std::uint64_t offset, char* dst, std::size_t max, std::size_t& got,
                               std::string& error) {
    got = 0;
    in_.clear();
    in_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    in_.read(dst, static_cast<std::streamsize>(max));
    got = static_cast<std::size_t>(in_.gcount());
    if (in_.bad()) {
        error = "read error on '" + path_.string() + "'";
        return false;
    }
    return true;
}

fs::path LocalFileDataSource::tempPath() const {
    fs::path p = path_;
    p += ".tmp~";
    return p;
}

bool LocalFileDataSource::beginWrite(std::string& error) {
    abortWrite();
    out_.open(tempPath(), std::ios::binary | std::ios::trunc);
    if (!out_) {
        error = "cannot create '" + tempPath().string() + "'";
        return false;
    }
    writing_ = true;
    return true;
}

bool LocalFileDataSource::writeChunk(std::string_view data, std::string& error) {
    out_.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!out_) {
        error = "write error on '" + tempPath().string() + "'";
        return false;
    }
    return true;
}

bool LocalFileDataSource::commitWrite(std::string& error) {
    out_.flush();
    const bool ok = static_cast<bool>(out_);
    out_.close();
    if (!ok) {
        error = "failed to flush '" + tempPath().string() + "'";
        abortWrite();
        return false;
    }
    std::error_code ec;
    fs::rename(tempPath(), path_, ec); // replaces an existing target
    writing_ = false;
    if (ec) {
        error = "cannot replace '" + path_.string() + "': " + ec.message();
        std::error_code ignore;
        fs::remove(tempPath(), ignore);
        return false;
    }
    return true;
}

void LocalFileDataSource::abortWrite() {
    if (out_.is_open()) out_.close();
    if (writing_) {
        std::error_code ec;
        fs::remove(tempPath(), ec);
        writing_ = false;
    }
}

} // namespace ced
