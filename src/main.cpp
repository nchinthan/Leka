#include "controller/MainController.h"
#include "renderer/WindowsTerminal.h"

#include <cstdio>
#include <exception>
#include <string>

int main(int argc, char** argv) {
    std::string loadError;
    try {
        {
            ced::WindowsTerminal terminal; // switches to its own screen buffer; restores the console on exit
            ced::MainController editor(terminal);
            if (argc > 1) editor.openFile(argv[1], loadError); // failure is also shown in the status bar
            editor.run();
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
    if (!loadError.empty()) {
        std::fprintf(stderr, "error: %s\n", loadError.c_str());
        return 1;
    }
    return 0;
}
