FIND_PATH(PUGIXML_INCLUDE_DIR NAMES pugixml.hpp)
FIND_LIBRARY(PUGIXML_LIBRARY pugixml)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PugiXML DEFAULT_MSG PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR)

mark_as_advanced(PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR)
