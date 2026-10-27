#pragma once
#include "input/KeyBindingRegistry.h"

namespace ced {

// Installs the stock bindings. Call it, then bind()/unbind() on top to customize.
void installDefaultKeyBindings(KeyBindingRegistry& registry);

} // namespace ced
