project(lemongrab)
cmake_minimum_required(VERSION 3.3.0 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3 -O0 -Wall")

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_COVER "Generate test coverage" OFF)
option(CHECK_INCLUDES "Run iwyu" OFF)

include(ClangTidy)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(Cpr REQUIRED)
find_package(Gloox REQUIRED)
find_package(Glog REQUIRED)
find_package(LibEvent REQUIRED)
find_package(JsonCpp REQUIRED)
find_package(LevelDB REQUIRED)
find_package(CppToml REQUIRED)
find_package(PugiXML REQUIRED)
find_package(Boost COMPONENTS locale REQUIRED)

find_program(iwyu_path NAMES include-what-you-use iwyu)
if(CHECK_INCLUDES AND NOT iwyu_path)
    message(FATAL_ERROR "Could not find the program include-what-you-use but CHECK_INCLUDES is ON")
endif()

include_directories(
    ${CPR_INCLUDE_DIRS}
    ${GLOOX_INCLUDE_DIR}
    ${GLOG_INCLUDE_DIR}
    ${CPPTOML_INCLUDE_DIR}
    ${JSONCPP_INCLUDE_DIR}
    ${PUGIXML_INCLUDE_DIR}
    ${LIBEVENT_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${LEVELDB_INCLUDE_DIR}
    )

file(GLOB_RECURSE SRC_LIST "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE TRAVIS_HELPERS "${CMAKE_SOURCE_DIR}/travis/*.sh")

add_executable(${PROJECT_NAME} ${SRC_LIST} ${TRAVIS_HELPERS} .travis.yml .clang-tidy update.sh README.md LICENSE config.toml.default)

if(BUILD_TESTS)
    add_definitions(-D_BUILD_TESTS)
    find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${GTEST_BOTH_LIBRARIES})
endif(BUILD_TESTS)

if(BUILD_COVER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    ${CPR_LIBRARIES}
    ${CURL_LIBRARIES}
    ${GLOOX_LIBRARY}
    ${GLOG_LIBRARY}
    ${JSONCPP_LIBRARY}
    ${PUGIXML_LIBRARY}
    ${LIBEVENT_LIBRARY}
    ${LIBEVENT_PTHREADS_LIBRARY}
    ${Boost_LIBRARIES}
    ${LEVELDB_LIBRARY}
    )

if (CHECK_INCLUDES)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
