#pragma once
#include "state/CommonHandlers.h"

namespace ced {
class InsertState final : public NavigableState {
public:
    InsertState();
    ModeId modeId() const override { return std::string(modes::Insert); }
    std::string displayName() const override { return "INSERT"; }
};
} // namespace ced
