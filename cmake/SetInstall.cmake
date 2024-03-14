message(STATUS "Setting Install")

# Install target Output Artifacts and associated files
install(
  TARGETS trantor
  # IMPORTANT: Add the trantor library to the "export-set",
  # ⚠️To actually install the export file itself, call install(EXPORT),
  EXPORT TrantorTargets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
)
# Install a CMake file exporting targets for dependent projects
install(
  EXPORT TrantorTargets
  DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}"
  NAMESPACE Trantor::
  COMPONENT dev
)

# Install header files
install(FILES ${public_net_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/net)
install(FILES ${public_utils_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/utils)
install(FILES ${trantor_export_header} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor)

# Install the package configuration and package version file and cmake module files
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TrantorConfigVersion.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/Findc-ares.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/FindBotan.cmake"
  DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}"
  COMPONENT dev
)

# Install the documentation. Don't install twice, so limit to Debug (assume developer)
if(BUILD_DOC)
  install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/${PROJECT_NAME}
    TYPE DOC
    CONFIGURATIONS Debug
  )
endif()
