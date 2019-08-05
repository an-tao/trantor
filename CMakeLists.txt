CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
PROJECT(Trantor)

set(TRANTOR_MAJOR_VERSION 1)
set(TRANTOR_MINOR_VERSION 0)
set(TRANTOR_PATCH_VERSION 0)
set(TRANTOR_VERSION
  ${TRANTOR_MAJOR_VERSION}.${TRANTOR_MINOR_VERSION}.${TRANTOR_PATCH_VERSION})

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_INCLUDE_DIR include CACHE PATH
  "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_TRANTOR_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_TRANTOR_CMAKE_DIR lib/CMake/Trantor)
endif()
set(INSTALL_TRANTOR_CMAKE_DIR ${DEF_INSTALL_TRANTOR_CMAKE_DIR} CACHE PATH
  "Installation directory for CMake files")

# Make relative paths absolute (needed later on)
foreach(p LIB INCLUDE TRANTOR_CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

# include directories
INCLUDE_DIRECTORIES(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/trantor/utils
        ${PROJECT_SOURCE_DIR}/trantor/net
        ${PROJECT_SOURCE_DIR}/trantor/net/inner
)

# lib directories
# LINK_DIRECTORIES(
#         ${PROJECT_BINARY_DIR}
# )

SET(trantor_sources
    trantor/utils/AsyncFileLogger.cc
    trantor/utils/ConcurrentTaskQueue.cc
    trantor/utils/Date.cc
    trantor/utils/LogStream.cc
    trantor/utils/Logger.cc
    trantor/utils/MsgBuffer.cc
    trantor/utils/SerialTaskQueue.cc
    trantor/utils/TimingWheel.cc
    trantor/net/EventLoop.cc
    trantor/net/EventLoopThread.cc
    trantor/net/EventLoopThreadPool.cc
    trantor/net/InetAddress.cc
    trantor/net/TcpClient.cc
    trantor/net/TcpServer.cc
    trantor/net/inner/Acceptor.cc
    trantor/net/inner/Channel.cc
    trantor/net/inner/Connector.cc
    trantor/net/inner/Poller.cc
    trantor/net/inner/Socket.cc
    trantor/net/inner/TcpConnectionImpl.cc
    trantor/net/inner/Timer.cc
    trantor/net/inner/TimerQueue.cc
    trantor/net/inner/poller/EpollPoller.cc
    trantor/net/inner/poller/KQueue.cc)

FIND_PACKAGE(OpenSSL)
IF(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    LINK_LIBRARIES(${OPENSSL_LIBRARIES})
    SET(trantor_sources ${trantor_sources} trantor/net/ssl/SSLConnection.cc)
ELSE()
    SET(trantor_sources ${trantor_sources} trantor/net/ssl/SSLConnectionSkipped.cc)
ENDIF()

ADD_LIBRARY(trantor ${trantor_sources})
SET_PROPERTY(TARGET trantor PROPERTY CXX_STANDARD 14)
SET_PROPERTY(TARGET trantor PROPERTY CXX_STANDARD_REQUIRED ON)
SET_PROPERTY(TARGET trantor PROPERTY CXX_EXTENSIONS OFF)

IF (MAKETEST STREQUAL YES)
    ADD_SUBDIRECTORY(trantor/tests)
ENDIF ()

SET(public_net_headers
    trantor/net/EventLoop.h
    trantor/net/EventLoopThread.h
    trantor/net/EventLoopThreadPool.h
    trantor/net/InetAddress.h
    trantor/net/TcpClient.h
    trantor/net/TcpConnection.h
    trantor/net/TcpServer.h
    trantor/net/callbacks.h)
    
SET(public_utils_headers
    trantor/utils/AsyncFileLogger.h
    trantor/utils/ConcurrentTaskQueue.h
    trantor/utils/Date.h
    trantor/utils/Funcs.h
    trantor/utils/LockFreeQueue.h
    trantor/utils/LogStream.h
    trantor/utils/Logger.h
    trantor/utils/MsgBuffer.h
    trantor/utils/NonCopyable.h
    trantor/utils/ObjectPool.h
    trantor/utils/SerialTaskQueue.h
    trantor/utils/TaskQueue.h
    trantor/utils/TimingWheel.h)

SOURCE_GROUP( "Public API" FILES ${public_net_headers} ${public_utils_headers})

install(TARGETS trantor
  # IMPORTANT: Add the trantor library to the "export-set"
  EXPORT TrantorTargets
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib)

INSTALL(FILES ${public_net_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/net)
INSTALL(FILES ${public_utils_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/utils)


# Add all targets to the build-tree export set
export(TARGETS trantor
  FILE "${PROJECT_BINARY_DIR}/TrantorTargets.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE Trantor)

# Create the TrantorConfig.cmake and TrantorConfigVersion files
file(RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_TRANTOR_CMAKE_DIR}"
   "${INSTALL_INCLUDE_DIR}")
# ... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(cmake/templates/TrantorConfig.cmake.in
  "${PROJECT_BINARY_DIR}/TrantorConfig.cmake" @ONLY)
# ... for the install tree
set(CONF_INCLUDE_DIRS "\${TRANTOR_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file(cmake/templates/TrantorConfig.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake" @ONLY)
# ... for both
configure_file(cmake/templates/TrantorConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/TrantorConfigVersion.cmake" @ONLY)

# Install the TrantorConfig.cmake and TrantorConfigVersion.cmake
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake"
  "${PROJECT_BINARY_DIR}/TrantorConfigVersion.cmake"
  DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}" COMPONENT dev)

# Install the export set for use with the install-tree
install(EXPORT TrantorTargets DESTINATION
  "${INSTALL_TRANTOR_CMAKE_DIR}" COMPONENT dev)

