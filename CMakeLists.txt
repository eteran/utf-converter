cmake_minimum_required(VERSION 3.0)
project(utf-converter CXX)

set(CMAKE_CXX_STANDARD 11)
find_package(Boost 1.35 REQUIRED)

include_directories(
	include
)

link_directories(
)

add_executable(utf-converter main.cpp)

target_link_libraries(utf-converter
)

enable_testing()
