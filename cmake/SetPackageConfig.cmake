message(STATUS "Setting Package config")

include(CMakePackageConfigHelpers)
# Generating a Package Configuration File
configure_package_config_file(
  cmake/templates/TrantorConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake
  INSTALL_DESTINATION ${INSTALL_TRANTOR_CMAKE_DIR}
)

# Generating a Package Version File
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/TrantorConfigVersion.cmake
  VERSION ${TRANTOR_VERSION}
  COMPATIBILITY SameMajorVersion
)
