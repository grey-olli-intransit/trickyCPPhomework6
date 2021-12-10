cmake_minimum_required(VERSION 3.20)
project(homework6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS " -pthread ${CMAKE_CXX_FLAGS}" )
add_executable(homework6 main.cpp)
target_include_directories(homework6 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
