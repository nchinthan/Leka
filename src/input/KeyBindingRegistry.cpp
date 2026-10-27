#include "input/KeyBindingRegistry.h"

namespace ced {

void KeyBindingRegistry::bind(std::string_view mode, KeyChord chord, ActionFactory factory) {
    exact_[std::string(mode)][chord] = std::move(factory);
}

void KeyBindingRegistry::bind(std::string_view mode, KeyChord chord, Action action) {
    bind(mode, chord, std::vector<Action>{std::move(action)});
}

void KeyBindingRegistry::bind(std::string_view mode, KeyChord chord, std::vector<Action> sequence) {
    bind(mode, chord, ActionFactory([seq = std::move(sequence)](const KeyEvent&) { return seq; }));
}

void KeyBindingRegistry::bindFallback(std::string_view mode, KeyPredicate when, ActionFactory factory) {
    fallbacks_[std::string(mode)].push_back({std::move(when), std::move(factory)});
}

bool KeyBindingRegistry::unbind(std::string_view mode, KeyChord chord) {
    const auto it = exact_.find(mode);
    return it != exact_.end() && it->second.erase(chord) > 0;
}

std::vector<Action> KeyBindingRegistry::resolve(const ModeId& mode, const KeyEvent& event) const {
    const KeyChord chord = KeyChord::from(event);
    for (std::string_view scope : {std::string_view(mode), modes::Global}) {
        const auto table = exact_.find(scope);
        if (table == exact_.end()) continue;
        const auto it = table->second.find(chord);
        if (it != table->second.end()) return it->second(event);
    }
    const auto fb = fallbacks_.find(mode);
    if (fb != fallbacks_.end())
        for (const Fallback& f : fb->second)
            if (f.when(event)) return f.make(event);
    return {};
}

} // namespace ced
