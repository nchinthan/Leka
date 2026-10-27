#pragma once
#include "document/TextProcessBackend.h"
#include "input/KeyBindingRegistry.h"
#include "renderer/ITerminal.h"
#include "renderer/Renderer.h"
#include "state/CommandRegistry.h"
#include "state/StateRegistry.h"
#include "state/ViewState.h"

#include <memory>
#include <string>

namespace ced {

// Coordinates input -> semantic actions -> active state -> document -> view -> renderer.
// Contains no piece-table logic and no drawing code; modes, key bindings and commands
// are all looked up in registries that callers may extend.
class MainController {
public:
    explicit MainController(ITerminal& terminal);

    bool openFile(const std::string& path, std::string& error);
    bool openSource(std::unique_ptr<IDataSource> source, std::string& error);

    void run();                                   // event loop until quit or input closes
    bool processEvent(const InputEvent& event);   // returns false when the editor should exit
    void renderNow();                             // render with the current invalidation (initial draw)

    // Extension points and inspection.
    KeyBindingRegistry& keys() { return keys_; }
    StateRegistry& states() { return states_; }
    CommandRegistry& commands() { return commands_; }
    TextProcessBackend& backend() { return backend_; }
    const ViewState& view() const { return view_; }
    const ModeId& activeMode() const { return activeMode_; }
    const std::string& statusMessage() const { return status_.message; }
    const Renderer& renderer() const { return renderer_; }
    bool quitRequested() const { return quit_; }

private:
    EditorContext context();
    ActionResult dispatch(const Action& action);
    bool transitionTo(const ModeId& mode);
    bool updateViewport();
    void refreshStatus();
    void draw(DirtyRegion region);

    ITerminal& terminal_;
    TextProcessBackend backend_;
    ViewState view_;
    CommandLine commandLine_;
    StateRegistry states_;
    CommandRegistry commands_;
    KeyBindingRegistry keys_;
    Renderer renderer_;
    StatusInfo status_;
    ModeId activeMode_;
    bool quit_ = false;
};

} // namespace ced
