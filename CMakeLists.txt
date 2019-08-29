cmake_minimum_required(VERSION 3.6)
project(trantor)
add_library(${PROJECT_NAME} STATIC)
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
  set(DEF_INSTALL_TRANTOR_CMAKE_DIR cmake)
else()
  set(DEF_INSTALL_TRANTOR_CMAKE_DIR lib/cmake/Trantor)
endif()
set(INSTALL_TRANTOR_CMAKE_DIR ${DEF_INSTALL_TRANTOR_CMAKE_DIR} CACHE PATH
  "Installation directory for cmake files")

# Make relative paths absolute (needed later on)
foreach(p LIB INCLUDE TRANTOR_CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

# include directories
target_include_directories(${PROJECT_NAME}
                           PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
                                  $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
                           PRIVATE ${PROJECT_SOURCE_DIR}
                                   ${PROJECT_SOURCE_DIR}/trantor/utils
                                   ${PROJECT_SOURCE_DIR}/trantor/net
                                   ${PROJECT_SOURCE_DIR}/trantor/net/inner
)

set(TRANTOR_SOURCES
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

find_package (OpenSSL)
if(OpenSSL_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    set(TRANTOR_SOURCES ${TRANTOR_SOURCES} trantor/net/ssl/SSLConnection.cc)
else()
    set(TRANTOR_SOURCES ${TRANTOR_SOURCES} trantor/net/ssl/SSLConnectionSkipped.cc)
endif()

find_path(CARES_INCLUDE_DIR ares.h)
find_library(CARES_LIBRARY NAMES cares)
if(CARES_INCLUDE_DIR AND CARES_LIBRARY)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CARES_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} INTERFACE ${CARES_LIBRARY})
    set(TRANTOR_SOURCES ${TRANTOR_SOURCES} trantor/net/inner/AresResolver.cc)
else()
    set(TRANTOR_SOURCES ${TRANTOR_SOURCES} trantor/net/inner/NormalResolver.cc)
endif()
target_sources(${PROJECT_NAME} PRIVATE ${TRANTOR_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
target_link_libraries(${PROJECT_NAME} INTERFACE pthread)

if(MAKETEST STREQUAL YES)
    add_subdirectory(trantor/tests)
    find_package(GTest)
    if(GTest_FOUND)
        add_subdirectory(trantor/unittests)
    endif()
endif()

set(public_net_headers
    trantor/net/EventLoop.h
    trantor/net/EventLoopThread.h
    trantor/net/EventLoopThreadPool.h
    trantor/net/InetAddress.h
    trantor/net/TcpClient.h
    trantor/net/TcpConnection.h
    trantor/net/TcpServer.h
    trantor/net/callbacks.h
    trantor/net/Resolver.h)
    
set(public_utils_headers
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

source_group( "Public API" FILES ${public_net_headers} ${public_utils_headers})

install(TARGETS trantor
  # IMPORTANT: Add the trantor library to the "export-set"
  EXPORT TrantorTargets
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib)

install(FILES ${public_net_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/net)
install(FILES ${public_utils_headers} DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/utils)


# Add all targets to the build-tree export set
export(TARGETS trantor
       FILE "${PROJECT_BINARY_DIR}/TrantorTargets.cmake")

include(CMakePackageConfigHelpers)
# ... for the build tree(locally included)
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}")
configure_package_config_file(cmake/templates/TrantorConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/TrantorConfig.cmake
                              INSTALL_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
                              PATH_VARS CONF_INCLUDE_DIRS)
# ... for the install tree
set(CONF_INCLUDE_DIRS "${INSTALL_INCLUDE_DIR}")
configure_package_config_file(cmake/templates/TrantorConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake
                              INSTALL_DESTINATION ${INSTALL_TRANTOR_CMAKE_DIR}
                              PATH_VARS CONF_INCLUDE_DIRS)

# version
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/TrantorConfigVersion.cmake
                                 VERSION ${TRANTOR_VERSION}
                                 COMPATIBILITY SameMajorVersion)

# Install the TrantorConfig.cmake and TrantorConfigVersion.cmake
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TrantorConfigVersion.cmake"
        DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}" COMPONENT dev)

# Install the export set for use with the install-tree
install(EXPORT TrantorTargets 
        DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}" COMPONENT dev)

