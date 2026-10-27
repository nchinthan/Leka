#pragma once
#include "state/EditorState.h"

#include <map>
#include <memory>
#include <vector>

namespace ced {

// Holds every available mode. New modes are added with add(); nothing else changes.
class StateRegistry {
public:
    void add(std::unique_ptr<EditorState> state) {
        const ModeId id = state->modeId();
        states_[id] = std::move(state);
    }
    EditorState* find(std::string_view mode) const {
        const auto it = states_.find(mode);
        return it == states_.end() ? nullptr : it->second.get();
    }
    bool contains(std::string_view mode) const { return find(mode) != nullptr; }
    std::vector<ModeId> modes() const {
        std::vector<ModeId> out;
        for (const auto& [id, _] : states_) out.push_back(id);
        return out;
    }

private:
    std::map<ModeId, std::unique_ptr<EditorState>, std::less<>> states_;
};

} // namespace ced
