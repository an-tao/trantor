macro(add_gtest)

  if(NOT googletest_DOWNLOAD_URL)
    get_github_latest_release_url("google" "googletest")
  endif()
  # turn off gmock
  set(BUILD_GMOCK OFF)

  FetchContent_Declare(googletest URL ${googletest_DOWNLOAD_URL})
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE
  )
  FetchContent_MakeAvailable(googletest)

  # Override default output directory set in googletest for gtest/gtest_main
  set_target_properties(
    gtest
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
               LIBRARY_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
               ARCHIVE_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
               PDB_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
  )
  set_target_properties(
    gtest_main
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
               LIBRARY_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
               ARCHIVE_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
               PDB_OUTPUT_DIRECTORY
               "${CMAKE_BINARY_DIR}"
  )

endmacro()

# ######################################################################################################################
if(BUILD_TESTING)
  message(STATUS "Setting Testing")
  find_package(GTest)

  if(NOT GTest_FOUND)
    if(FETCH_BUILD_MISSING_DEPS)
      message(STATUS "⚠️GTest not found, Set it with FetchContent...")
      unset(GTest_FOUND)

      add_gtest()
    else()
      message(FATAL_ERROR "⛔GTest not found, please install it or set FETCH_BUILD_MISSING_DEPS to ON")
    endif()
  endif()

  enable_testing()
  # unittests
  add_subdirectory(trantor/unittests)
  # tests
  add_subdirectory(trantor/tests)
endif()
