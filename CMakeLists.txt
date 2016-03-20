cmake_minimum_required(VERSION 3.0)
project(utf-converter)

find_package(Boost 1.35 COMPONENTS program_options REQUIRED)

set(CMAKE_CXX_STANDARD 11)

include_directories(
	include
	${Boost_INCLUDE_DIR}
)

link_directories(
	${Boost_LIBRARY_DIR}
)

add_executable(utf-converter main.cpp)

target_link_libraries(utf-converter
	${Boost_LIBRARIES}
)

enable_testing()
