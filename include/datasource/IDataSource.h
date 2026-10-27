#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ced {

// Where document bytes come from and go to. Knows nothing about piece tables.
// Reads are offset-based so a future remote/paged source can serve ranges on demand.
class IDataSource {
public:
    virtual ~IDataSource() = default;

    virtual std::string name() const = 0;               // human-readable identity (e.g. a file name)
    virtual bool exists() const = 0;                    // false => treat as a new, empty document
    virtual bool open(std::string& error) = 0;          // prepare for reading
    virtual std::optional<std::uint64_t> size() const = 0;
    // Reads up to `max` bytes at `offset`. `got == 0` with success means end of data.
    virtual bool read(std::uint64_t offset, char* dst, std::size_t max, std::size_t& got, std::string& error) = 0;

    virtual bool supportsWrite() const = 0;
    virtual bool beginWrite(std::string& error) = 0;
    virtual bool writeChunk(std::string_view data, std::string& error) = 0;
    virtual bool commitWrite(std::string& error) = 0;   // atomically publish what was written
    virtual void abortWrite() = 0;                      // discard partial output
};

} // namespace ced
