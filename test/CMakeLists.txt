add_executable(game_tests
    Gtest.cpp
    ../gamemaster/gamelogic/GameLogic.cpp
    ../gamemaster/gamelogic/Player.cpp
)

# Include paths for header files
target_include_directories(game_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/gamemaster/gamelogic
    ${CMAKE_SOURCE_DIR}/common
)

# Link GTest and nlohmann_json
target_link_libraries(game_tests PRIVATE
    gtest_main
    nlohmann_json::nlohmann_json
)

include(GoogleTest)
gtest_discover_tests(game_tests)

# --- Balance Simulator ---
add_executable(balance_simulator
    balance_simulator.cpp
    ../gamemaster/gamelogic/GameLogic.cpp
    ../gamemaster/gamelogic/Player.cpp
)

target_include_directories(balance_simulator PRIVATE 
    ${CMAKE_SOURCE_DIR}/gamemaster/gamelogic
    ${CMAKE_SOURCE_DIR}/common
)

target_link_libraries(balance_simulator PRIVATE
    nlohmann_json::nlohmann_json
)
