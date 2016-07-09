# additional target to perform clang-format run, requires clang-format
# get all project files

file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp)

add_custom_target( clang-tidy COMMAND clang-tidy -p=${CMAKE_BINARY_DIR}/compile_commands.json -header-filter=src/ -config= ${ALL_SOURCE_FILES} )
add_custom_target( clang-tidy-fix COMMAND clang-tidy -fix -p=${CMAKE_BINARY_DIR}/compile_commands.json -header-filter=src/ -config= ${ALL_SOURCE_FILES} )
