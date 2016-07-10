# additional target to perform clang-format run, requires clang-format
# get all project files

file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp)

add_custom_target(clang-tidy)
add_custom_target(clang-tidy-fix)

foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
  add_custom_command(TARGET clang-tidy PRE_BUILD
                     COMMAND clang-tidy -p=${CMAKE_BINARY_DIR}/compile_commands.json -header-filter=src/ -config= ${SOURCE_FILE})
endforeach()

foreach(SOURCE_FILE ${ALL_SOURCE_FILES})
  add_custom_command(TARGET clang-tidy-fix PRE_BUILD
                     COMMAND clang-tidy -fix -fix-errors -p=${CMAKE_BINARY_DIR}/compile_commands.json -header-filter=src/ -config= ${SOURCE_FILE})
endforeach()
