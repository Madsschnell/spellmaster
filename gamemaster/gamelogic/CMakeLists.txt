find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(nlohmann_json REQUIRED)

add_executable(gamemaster
    main.cpp
    GameLogic.cpp
    Player.cpp
    Spell.cpp
)

target_include_directories(gamemaster PRIVATE ${ZMQ_INCLUDE_DIRS})
target_link_libraries(gamemaster ${ZMQ_LIBRARIES} nlohmann_json::nlohmann_json)
