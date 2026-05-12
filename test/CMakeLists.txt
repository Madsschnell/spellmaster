add_executable(game_tests
    Gtest.cpp
    ../gamemaster/gamelogic/GameLogic.cpp
    ../gamemaster/gamelogic/Player.cpp
)

# Fortæl testen hvor den kan finde header-filerne (.h)
target_include_directories(game_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/gamemaster/gamelogic
    ${CMAKE_SOURCE_DIR}/common
)

# Link GTest og nlohmann_json
target_link_libraries(game_tests PRIVATE
    gtest_main
    nlohmann_json::nlohmann_json
)