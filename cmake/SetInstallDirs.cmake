message(STATUS "Setting Install Dirs")

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
