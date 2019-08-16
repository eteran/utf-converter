cmake_minimum_required(VERSION 3.2)

add_library(utf-converter-headers INTERFACE)

target_sources(utf-converter-headers INTERFACE 
	${CMAKE_CURRENT_SOURCE_DIR}/utf_converter.h
)

target_include_directories(utf-converter-headers INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}
)

