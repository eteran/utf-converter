cmake_minimum_required(VERSION 3.2)

add_executable(utf-converter
	main.cpp
)

target_compile_options(utf-converter PRIVATE
	-W
	-Wall
	-pedantic
)

target_link_libraries(utf-converter PUBLIC
	utf-converter-headers
)

set_property(TARGET utf-converter PROPERTY CXX_STANDARD 17)
set_property(TARGET utf-converter PROPERTY CXX_EXTENSIONS OFF)

add_subdirectory(include)
