cmake_minimum_required(VERSION 3.16)
project(SpellMaster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# nlohmann_json
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# cppzmq (zmq.hpp)
find_package(cppzmq CONFIG QUIET)
if(NOT cppzmq_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        cppzmq
        URL https://github.com/zeromq/cppzmq/archive/refs/tags/v4.11.0.tar.gz
    )
    FetchContent_MakeAvailable(cppzmq)
endif()

# Make zmq.hpp include path available globally, matching current include_directories style.
if(TARGET cppzmq)
    get_target_property(CPPZMQ_INCLUDE_DIRS cppzmq INTERFACE_INCLUDE_DIRECTORIES)
    if(CPPZMQ_INCLUDE_DIRS)
        include_directories(${CPPZMQ_INCLUDE_DIRS})
    endif()
endif()

# Delte headers (gamestatus.h, spellmessage.h, serialization.h, transport.h)
include_directories(${CMAKE_SOURCE_DIR}/common)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

# Player-komponenter (UI + ImageProcessing)
if(EXISTS ${CMAKE_SOURCE_DIR}/player)
    add_subdirectory(player/ui)
    add_subdirectory(player)
endif()

# Gamemaster-komponenter (GameLogic)
if(EXISTS ${CMAKE_SOURCE_DIR}/gamemaster)
    add_subdirectory(gamemaster/gamelogic)
endif()

# Tests
if(EXISTS ${CMAKE_SOURCE_DIR}/test)
    add_subdirectory(test)
endif()