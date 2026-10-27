#pragma once
#include <string>
#include <vector>

namespace ced {

// What is currently on the terminal (presentation state), kept apart from logical editor state.
class RenderedScreen {
public:
    void reset(int width, int height) {
        width_ = width;
        height_ = height;
        rows_.assign(static_cast<std::size_t>(height), std::string(static_cast<std::size_t>(width), ' '));
    }
    int width() const { return width_; }
    int height() const { return height_; }
    std::string& row(int r) { return rows_[static_cast<std::size_t>(r)]; }
    const std::string& row(int r) const { return rows_[static_cast<std::size_t>(r)]; }

private:
    int width_ = 0, height_ = 0;
    std::vector<std::string> rows_;
};

} // namespace ced
