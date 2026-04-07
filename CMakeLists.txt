cmake_minimum_required(VERSION 3.16)
project(SpellMaster LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Delte headers (gamestatus.h, spellmessage.h, serialization.h, transport.h)
include_directories(${CMAKE_SOURCE_DIR}/common)

# Player-komponenter (UI + ImageProcessing)
if(EXISTS ${CMAKE_SOURCE_DIR}/player)
    add_subdirectory(player/ui)
    # add_subdirectory(player/imageprocessing)  # aktiveres når koden er klar
endif()

# Gamemaster-komponenter (GameLogic)
if(EXISTS ${CMAKE_SOURCE_DIR}/gamemaster)
    # add_subdirectory(gamemaster/gamelogic)  # aktiveres når koden er klar
endif()
