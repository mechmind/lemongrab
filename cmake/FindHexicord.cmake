FIND_PATH(HEXICORD_INCLUDE_DIR NAMES hexicord/config.hpp)
FIND_LIBRARY(HEXICORD_LIBRARY hexicord)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Hexicord DEFAULT_MSG HEXICORD_LIBRARY HEXICORD_INCLUDE_DIR)

mark_as_advanced(HEXICORD_LIBRARY HEXICORD_INCLUDE_DIR)
