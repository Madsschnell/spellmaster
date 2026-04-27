find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(nlohmann_json REQUIRED)

add_executable(spellmaster_player main.cpp)
target_include_directories(spellmaster_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZMQ_INCLUDE_DIRS}
)
target_link_libraries(spellmaster_player
    spellmaster_ui_lib
    ${ZMQ_LIBRARIES}
    nlohmann_json::nlohmann_json
)
