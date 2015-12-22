project(lemongrab)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3 -O0 -Wall")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pthread" )

find_package(PkgConfig)
INCLUDE(FindCURL)
pkg_search_module(GLOOX gloox)

INCLUDE_DIRECTORIES(. ${CURL_INCLUDE_DIR} ${GLOOX_INCLUDE_DIR})

FILE(GLOB_RECURSE SRC_LIST *.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CURL_LIBRARY} ${GLOOX_LDFLAGS})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)