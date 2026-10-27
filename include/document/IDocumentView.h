#pragma once
#include <cstddef>
#include <string>

namespace ced {

// Read-only view of a document. This is all the renderer ever sees.
// Lines are zero-based. An empty document has exactly one (empty) line.
class IDocumentView {
public:
    virtual ~IDocumentView() = default;
    virtual std::size_t size() const = 0;
    virtual std::size_t lineCount() const = 0;
    virtual std::size_t lineStartOffset(std::size_t line) const = 0;
    virtual std::size_t lineOfOffset(std::size_t offset) const = 0;
    virtual std::size_t lineLength(std::size_t line) const = 0; // excludes the '\n'
    virtual std::string lineText(std::size_t line) const = 0;   // excludes the '\n'
    virtual std::string readRange(std::size_t position, std::size_t length) const = 0;
};

} // namespace ced
