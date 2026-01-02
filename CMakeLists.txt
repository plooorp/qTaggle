cmake_minimum_required(VERSION 3.16)
project(qtaggle
	DESCRIPTION "File tagging utility"
	VERSION 1.0.0
	LANGUAGES CXX C
)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(src)

# auto-generated ui headers
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
