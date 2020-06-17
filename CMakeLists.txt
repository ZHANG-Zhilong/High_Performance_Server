cmake_minimum_required(VERSION 3.16)
project(High_Performance_Server)

set(CMAKE_CXX_STANDARD 14)
include_directories(${PROJECT_SOURCE_DIR})
add_subdirectory(src/base)
add_subdirectory(src/net)
add_subdirectory(src)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

add_executable(a.out main.cpp)
target_link_libraries(a.out server_server)

