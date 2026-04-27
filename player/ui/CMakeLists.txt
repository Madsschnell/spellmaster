find_package(Qt6 REQUIRED COMPONENTS Widgets)

# UI as a library (used by both standalone test and player application)
add_library(spellmaster_ui_lib STATIC SpellMasterUI.cpp)
target_include_directories(spellmaster_ui_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(spellmaster_ui_lib PUBLIC Qt6::Widgets)

# Standalone render test (no network)
add_executable(spellmaster_ui main.cpp)
target_link_libraries(spellmaster_ui spellmaster_ui_lib)
