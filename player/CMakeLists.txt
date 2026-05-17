find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(nlohmann_json REQUIRED)

# OpenCV: required for camera-based gesture recognition
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

set(PLAYER_SOURCES
    main.cpp
    imageprocessing/ImageProcessing.cpp
    imageprocessing/GestureClassifier.cpp
)

add_executable(spellmaster_player ${PLAYER_SOURCES})

target_include_directories(spellmaster_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZMQ_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(spellmaster_player
    spellmaster_ui_lib
    ${ZMQ_LIBRARIES}
    nlohmann_json::nlohmann_json
    ${OpenCV_LIBS}
)
