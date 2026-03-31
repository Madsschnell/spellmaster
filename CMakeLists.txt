cmake_minimum_required(VERSION 3.10)
project(camera_stream)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCAMERA REQUIRED libcamera)

add_executable(camera_stream camera_stream.cpp)

target_include_directories(camera_stream PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${LIBCAMERA_INCLUDE_DIRS}
)

target_link_libraries(camera_stream
    ${OpenCV_LIBS}
    ${LIBCAMERA_LIBRARIES}
)