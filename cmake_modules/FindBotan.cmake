
find_package(PkgConfig)
if(NOT WIN32 AND PKG_CONFIG_FOUND)
  if (NOT TARGET Botan::Botan)
    pkg_check_modules(Botan QUIET IMPORTED_TARGET botan-2)
    if (TARGET PkgConfig::Botan)
      add_library(Botan::Botan ALIAS PkgConfig::Botan)
    endif ()
  endif()
endif()

if (NOT TARGET Botan::Botan)
  find_path(Botan_INCLUDE_DIRS NAMES botan/botan.h
            PATH_SUFFIXES botan-2
            DOC "The Botan include directory")

  find_library(Botan_LIBRARIES NAMES botan botan-2
               DOC "The Botan library")

  mark_as_advanced(Botan_INCLUDE_DIRS Botan_LIBRARIES)

  add_library(Botan::Botan IMPORTED UNKNOWN)
  set_target_properties(
    Botan::Botan
    PROPERTIES
    IMPORTED_LOCATION "${Botan_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${Botan_INCLUDE_DIRS}"
  )

  if (WIN32)
    target_compile_definitions(Botan::Botan INTERFACE -DNOMINMAX=1)
  endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Botan
  REQUIRED_VARS Botan_LIBRARIES Botan_INCLUDE_DIRS
)
