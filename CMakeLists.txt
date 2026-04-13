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

# Finder ZeroMQ (kræver at cppzmq-dev er installeret)
find_package(cppzmq REQUIRED)

# Henter nlohmann/json automatisk ned via nettet, så det virker på alle computere
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)
target_link_libraries(GameMasterApp PRIVATE nlohmann_json::nlohmann_json cppzmq)
