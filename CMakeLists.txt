project(lemongrab)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3 -O0 -Wall")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pthread" )

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_COVER "Generate test coverage" OFF)

find_package(PkgConfig)
include(FindCURL)
pkg_search_module(GLOOX gloox)
pkg_search_module(LIBEVENT libevent)
# find_package(Boost 1.55 COMPONENTS locale)

include_directories(. ${CURL_INCLUDE_DIR} ${GLOOX_INCLUDE_DIR} ${LIBEVENT_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

file(GLOB_RECURSE SRC_LIST "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC_LIST})

if(BUILD_TESTS)
    add_definitions(-D_BUILD_TESTS)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${GTEST_BOTH_LIBRARIES})
endif(BUILD_TESTS)

if(BUILD_COVER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARY} ${GLOOX_LDFLAGS} ${LIBEVENT_LDFLAGS} ${Boost_LIBRARIES} -lboost_locale -lleveldb -ljsoncpp)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
