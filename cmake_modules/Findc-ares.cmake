#[[
# Try to find c-ares library Once done this will define
#
# c-ares_FOUND - system has c-ares
# c-ares_INCLUDE_DIRS - The c-ares include directory
# c-ares_LIBRARIES - Link these to use c-ares
# c-ares_lib - Imported Targets
#
# Copyright (c) 2020 antao <antao2002@gmail.com>
#]]

find_path(c-ares_INCLUDE_DIRS ares.h)
find_library(c-ares_LIBRARIES NAMES cares)
if(c-ares_INCLUDE_DIRS AND c-ares_LIBRARIES)
  add_library(c-ares_lib INTERFACE IMPORTED)
  set_target_properties(
    c-ares_lib
    PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${c-ares_INCLUDE_DIRS}" INTERFACE_LINK_LIBRARIES "${c-ares_LIBRARIES}"
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  c-ares
  DEFAULT_MSG
  c-ares_INCLUDE_DIRS
  c-ares_LIBRARIES
)
mark_as_advanced(c-ares_INCLUDE_DIRS c-ares_LIBRARIES)
