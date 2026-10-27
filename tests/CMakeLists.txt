add_executable(cmd_editor_tests
    TestMain.cpp
    test_piece_table.cpp
    test_line_index.cpp
    test_history.cpp
    test_state_transitions.cpp
    test_key_mapping.cpp
    test_rendering.cpp
)
target_link_libraries(cmd_editor_tests PRIVATE cmd_editor_core)
target_include_directories(cmd_editor_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    target_compile_options(cmd_editor_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(cmd_editor_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

foreach(suite IN ITEMS piece_table line_index history state_transitions key_mapping rendering)
    add_test(NAME ${suite} COMMAND cmd_editor_tests ${suite})
endforeach()
