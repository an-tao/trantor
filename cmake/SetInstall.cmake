message(STATUS "Setting Install")

include(GNUInstallDirs)
# Offer the user the choice of overriding the installation directories
set(INSTALL_BIN_DIR
    ${CMAKE_INSTALL_BINDIR}
    CACHE PATH "Installation directory for binaries"
)
set(INSTALL_LIB_DIR
    ${CMAKE_INSTALL_LIBDIR}
    CACHE PATH "Installation directory for libraries"
)
set(INSTALL_INCLUDE_DIR
    ${CMAKE_INSTALL_INCLUDEDIR}
    CACHE PATH "Installation directory for header files"
)
set(INSTALL_TRANTOR_CMAKE_DIR
    ${CMAKE_INSTALL_LIBDIR}/cmake/Trantor
    CACHE PATH "Installation directory for cmake files"
)

if(BUILD_SHARED_LIBS
   AND (NOT
        "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}"
        IN_LIST
        CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
       )
)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${INSTALL_LIB_DIR}")
endif()

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

# If build with spdlog
if(TARGET spdlog)
  install(
    TARGETS spdlog
    # IMPORTANT: Add the trantor library to the "export-set",
    # ⚠️To actually install the export file itself, call install(EXPORT),
    EXPORT TrantorTargets
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
    ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
    LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib
  )
endif()

# Install a CMake file exporting targets for dependent projects
install(
  EXPORT TrantorTargets
  DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}"
  NAMESPACE Trantor::
  COMPONENT dev
)

# Install header files
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/exports/trantor/exports.h DESTINATION ${INSTALL_INCLUDE_DIR}/trantor)
install(FILES ${public_net_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/net)
install(FILES ${public_utils_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/utils)
install(FILES ${TRANTOR_EXPORT_HEADER} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor)

# Install the package configuration and package version file and cmake module files
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TrantorConfigVersion.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/Findc-ares.cmake"
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
