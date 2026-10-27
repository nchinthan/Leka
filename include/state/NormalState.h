#pragma once
#include "state/CommonHandlers.h"

namespace ced {
class NormalState final : public NavigableState {
public:
    NormalState();
    ModeId modeId() const override { return std::string(modes::Normal); }
    std::string displayName() const override { return "NORMAL"; }
};
} // namespace ced
