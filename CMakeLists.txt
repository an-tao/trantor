cmake_minimum_required(VERSION 3.5)
project(trantor)
add_library(${PROJECT_NAME} STATIC)
# if(MSVC) target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX) else()
# target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
# endif()

set(TRANTOR_MAJOR_VERSION 1)
set(TRANTOR_MINOR_VERSION 0)
set(TRANTOR_PATCH_VERSION 0)
set(TRANTOR_VERSION
    ${TRANTOR_MAJOR_VERSION}.${TRANTOR_MINOR_VERSION}.${TRANTOR_PATCH_VERSION})


# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_INCLUDE_DIR
    include
    CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_TRANTOR_CMAKE_DIR cmake)
else()
  set(DEF_INSTALL_TRANTOR_CMAKE_DIR lib/cmake/Trantor)
endif()
set(INSTALL_TRANTOR_CMAKE_DIR
    ${DEF_INSTALL_TRANTOR_CMAKE_DIR}
    CACHE PATH "Installation directory for cmake files")

# Make relative paths absolute (needed later on)
foreach(p
        LIB
        INCLUDE
        TRANTOR_CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

if(WIN32)
  add_subdirectory(third_party/wepoll)
endif(WIN32)

# include directories
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
         $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
  PRIVATE ${PROJECT_SOURCE_DIR}
          ${PROJECT_SOURCE_DIR}/trantor/utils
          ${PROJECT_SOURCE_DIR}/trantor/net
          ${PROJECT_SOURCE_DIR}/trantor/net/inner
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/wepoll>)

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

if(WIN32)
  set(TRANTOR_SOURCES ${TRANTOR_SOURCES} trantor/utils/WindowsSupport.cc)
endif(WIN32)

find_package(OpenSSL)
if(OpenSSL_FOUND)
  target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})
  add_definitions(-DUSE_OPENSSL)
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

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 Rpcrt4 wepoll)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif(WIN32)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(${PROJECT_NAME} PROPERTIES EXPORT_NAME Trantor)

if(BUILD_TESTING)
  add_subdirectory(trantor/tests)
  find_package(GTest)
  if(GTest_FOUND)
    enable_testing()
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

source_group("Public API"
             FILES
             ${public_net_headers}
             ${public_utils_headers})

install(TARGETS trantor
                # IMPORTANT: Add the trantor library to the "export-set"
        EXPORT TrantorTargets
        ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT lib)

install(FILES ${public_net_headers}
        DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/net)
install(FILES ${public_utils_headers}
        DESTINATION ${INSTALL_INCLUDE_DIR}/trantor/utils)

include(CMakePackageConfigHelpers)
# ... for the install tree
configure_package_config_file(
  cmake/templates/TrantorConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake
  INSTALL_DESTINATION
  ${INSTALL_TRANTOR_CMAKE_DIR})

# version
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/TrantorConfigVersion.cmake
  VERSION ${TRANTOR_VERSION}
  COMPATIBILITY SameMajorVersion)

# Install the TrantorConfig.cmake and TrantorConfigVersion.cmake
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TrantorConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/TrantorConfigVersion.cmake"
  DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}"
  COMPONENT dev)

# Install the export set for use with the install-tree
install(EXPORT TrantorTargets
        DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}"
        NAMESPACE Trantor::
        COMPONENT dev)
