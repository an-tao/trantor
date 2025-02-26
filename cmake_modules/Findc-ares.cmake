#[[
# Try to find c-ares library Once done this will define
#
# c-ares_FOUND - system has c-ares
# C-ARES_INCLUDE_DIRS - The c-ares include directory
# C-ARES_LIBRARIES - Link these to use c-ares
# c-ares - Imported Targets
#
# Copyright (c) 2020 antao <antao2002@gmail.com>
#]]

find_path(
  C-ARES_INCLUDE_DIRS
  NAMES ares.h
  HINTS ${C-ARES_DIR}/include
)
find_library(
  C-ARES_LIBRARIES
  NAMES cares
  HINTS ${C-ARES_DIR}/lib
)
if(C-ARES_INCLUDE_DIRS AND C-ARES_LIBRARIES)
  if(NOT TARGET c-ares::cares)
    add_library(c-ares::cares INTERFACE IMPORTED)
    set_target_properties(
      c-ares::cares
      PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${C-ARES_INCLUDE_DIRS}" INTERFACE_LINK_LIBRARIES "${C-ARES_LIBRARIES}"
    )
  endif()
endif()
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  c-ares
  DEFAULT_MSG
  C-ARES_INCLUDE_DIRS
  C-ARES_LIBRARIES
)
mark_as_advanced(C-ARES_INCLUDE_DIRS C-ARES_LIBRARIES)
