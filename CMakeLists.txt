cmake_minimum_required(VERSION 3.16)
project(qtaggle LANGUAGES CXX C)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(sqlite3 STATIC ${PROJECT_SOURCE_DIR}/lib/sqlite3/sqlite3.c)

add_subdirectory(src)
