project(lemongrab)
cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3 -O0 -Wall")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pthread" )
set(CXX_INCLUDE_WHAT_YOU_USE "/usr/bin/include-what-you-use")

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_COVER "Generate test coverage" OFF)
option(CHECK_INCLUDES "Run iwyu" OFF)
SET(GLOOXPATH "" CACHE STRING "Path to Gloox library")

find_package(PkgConfig)
include(FindCURL)

if (GLOOXPATH STREQUAL "")
    pkg_search_module(GLOOX gloox)
else()
    SET(GLOOX_INCLUDE_DIRS "${GLOOXPATH}/src")
    SET(GLOOX_LDFLAGS "-L${GLOOXPATH}/.libs -lgloox")
endif()

pkg_search_module(LIBEVENT libevent_pthreads)
pkg_search_module(GLOG libglog)
pkg_search_module(JSONCPP jsoncpp)
# find_package(Boost 1.55 COMPONENTS locale)

find_program(iwyu_path NAMES include-what-you-use iwyu)
if(CHECK_INCLUDES AND NOT iwyu_path)
    message(FATAL_ERROR "Could not find the program include-what-you-use but CHECK_INCLUDES is ON")
endif()

include_directories(. ${CURL_INCLUDE_DIRS} ${GLOOX_INCLUDE_DIRS} ${GLOG_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS} ${LIBEVENT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

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

target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARY} ${GLOOX_LDFLAGS} ${GLOG_LDFLAGS} ${JSONCPP_LDFLAGS} ${LIBEVENT_LDFLAGS} ${Boost_LIBRARIES} -lboost_locale -lleveldb)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

if (CHECK_INCLUDES)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()
