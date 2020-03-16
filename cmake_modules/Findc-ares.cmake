# - Try to find c-ares library
# Once done this will define
#
# c-ares_FOUND - system has c-ares
# C-ARES_INCLUDE_DIRS - the c-ares include directory
# C-ARES_LIBRARIES - Link these to use c-ares
#
# Copyright (c) 2020 antao <antao2002@gmail.com>
#

find_path(C-ARES_INCLUDE_DIR ares.h)
find_library(C-ARES_LIBRARY NAMES cares)
if(C-ARES_INCLUDE_DIR AND C-ARES_LIBRARY)
  add_library(c-ares_lib INTERFACE IMPORTED)
  set_target_properties(c-ares_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   ${C-ARES_INCLUDE_DIR}
                                   INTERFACE_LINK_LIBRARIES
                                   ${C-ARES_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(c-ares DEFAULT_MSG
	C-ARES_INCLUDE_DIR C-ARES_LIBRARY)
mark_as_advanced (C-ARES_INCLUDE_DIR C-ARES_LIBRARY)
