cmake_minimum_required(VERSION 3.20)
project(cmd_editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Portable core: everything except the Win32 console and main() -----------------
set(CED_ACTION_SOURCES)  # header-only vocabulary (Action/ActionResult)
set(CED_INPUT_SOURCES
    src/input/KeyBindingRegistry.cpp
    src/input/KeyMap.cpp)
set(CED_STATE_SOURCES
    src/state/CommandRegistry.cpp
    src/state/CommandState.cpp
    src/state/CommonHandlers.cpp
    src/state/CursorOps.cpp
    src/state/InsertState.cpp
    src/state/NormalState.cpp)
set(CED_DOCUMENT_SOURCES
    src/document/AugmentedPieceTable.cpp
    src/document/HistoryManager.cpp
    src/document/TextProcessBackend.cpp)
set(CED_DATASOURCE_SOURCES
    src/datasource/LocalFileDataSource.cpp)
set(CED_RENDERER_SOURCES
    src/renderer/DirtyRegion.cpp
    src/renderer/Renderer.cpp
    src/renderer/TextLayout.cpp)
set(CED_CONTROLLER_SOURCES
    src/controller/MainController.cpp)

add_library(cmd_editor_core STATIC
    ${CED_INPUT_SOURCES}
    ${CED_STATE_SOURCES}
    ${CED_DOCUMENT_SOURCES}
    ${CED_DATASOURCE_SOURCES}
    ${CED_RENDERER_SOURCES}
    ${CED_CONTROLLER_SOURCES})

target_include_directories(cmd_editor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(cmd_editor_core PUBLIC /W4 /permissive- /utf-8)
else()
    target_compile_options(cmd_editor_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

# ---- The editor itself (Windows console) --------------------------------------------
if(WIN32)
    add_executable(cmd_editor
        src/main.cpp
        src/renderer/WindowsTerminal.cpp)
    target_link_libraries(cmd_editor PRIVATE cmd_editor_core)
    target_compile_definitions(cmd_editor PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    message(STATUS "cmd_editor executable is Windows-only; building core library and tests only")
endif()

# ---- Tests ----------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
