cmake_minimum_required(VERSION 3.10)
project(SpellmasterGM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Inkluderer 'include' mappen
include_directories(include)

# Tilføjer alle .cpp filer fra 'src' mappen
file(GLOB SOURCES "src/*.cpp")

# Skaber det kørbare program
add_executable(GameMasterApp ${SOURCES})
