function(find_botan_pkgconfig package_name)
  if(TARGET Botan::Botan)
    return()
  endif()

  pkg_check_modules(
    Botan
    QUIET
    IMPORTED_TARGET
    GLOBAL
    ${package_name}
  )
  if(TARGET PkgConfig::Botan)
    add_library(Botan::Botan ALIAS PkgConfig::Botan)

    if(${Botan_FIND_VERSION} EQUAL 3)
      target_compile_features(PkgConfig::Botan INTERFACE cxx_std_20)
    endif()
  endif()
endfunction()

function(find_botan_search package_name)
  # botan2 have botan.h, but botan3 does not, botan3 using auto_rng.h instead
  find_path(
    Botan_INCLUDE_DIRS
    NAMES botan/botan.h botan/auto_rng.h
    HINTS ${BOTAN_ROOT_DIR}/include
    PATH_SUFFIXES ${package_name}
    DOC "The Botan include directory"
  )

  find_library(
    Botan_LIBRARIES
    NAMES botan ${package_name}
    HINTS ${BOTAN_ROOT_DIR}/lib
    DOC "The Botan library"
  )

  if(Botan_INCLUDE_DIRS AND Botan_LIBRARIES)
    add_library(Botan::Botan IMPORTED UNKNOWN)
    set_target_properties(
      Botan::Botan
      PROPERTIES
      IMPORTED_LOCATION "${Botan_LIBRARIES}"
      INTERFACE_INCLUDE_DIRECTORIES "${Botan_INCLUDE_DIRS}"
    )
    if(${Botan_FIND_VERSION} EQUAL 3)
      target_compile_features(Botan::Botan INTERFACE cxx_std_20)
    endif()

    if(WIN32)
      target_compile_definitions(Botan::Botan INTERFACE -DNOMINMAX=1)
    endif()
  endif()
endfunction()

# ######################################################################################################################
# Using find_package with verion: find_package(Botan 3) or find_package(Botan 2)
if(NOT DEFINED Botan_FIND_VERSION)
  message(FATAL_ERROR "⛔Must specify Botan version: find_package(Botan 3) or find_package(Botan 2)")
endif()

if(NOT WIN32)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    find_botan_pkgconfig(botan-${Botan_FIND_VERSION})
  endif()
endif()

if(NOT TARGET Botan::Botan)
  find_botan_search(botan-${Botan_FIND_VERSION})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Botan REQUIRED_VARS Botan_LIBRARIES Botan_INCLUDE_DIRS)
mark_as_advanced(Botan_INCLUDE_DIRS Botan_LIBRARIES)
