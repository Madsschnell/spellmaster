cmake_minimum_required(VERSION 3.10)
project(spellmaster)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Helps VS Code / clangd / IntelliSense discover include paths.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)

add_executable(spellmaster
    main.cpp
    gesture_classifier.cpp
)

target_include_directories(spellmaster PRIVATE ${OpenCV_INCLUDE_DIRS})

target_link_libraries(spellmaster PRIVATE ${OpenCV_LIBS})