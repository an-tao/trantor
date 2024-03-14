if(BUILD_TESTING)
  message(STATUS "Setting Testing")
  # tests
  add_subdirectory(trantor/tests)
  # unittests
  enable_testing()
  add_subdirectory(trantor/unittests)
endif()
