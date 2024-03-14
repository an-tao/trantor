# CMake version < 3.14, could not use option BUILD_DEPENDENCIES
cmake_minimum_required(VERSION 3.10)
project(trantor)
add_library(${PROJECT_NAME})

# C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

# Build options
option(BUILD_SHARED_LIBS "Build Trantor as a shared lib" OFF)
option(BUILD_TESTING "Build tests" OFF)
option(BUILD_DOC "Build Doxygen documentation" OFF)
option(BUILD_DEPENDENCIES "Fetch and build dependencies" OFF)
option(TRANTOR_USE_SPDLOG "Allow using the spdlog logging library" ON)
option(TRANTOR_USE_C-ARES "Allow using C-ARES" ON)
set(TRANTOR_TLS_PROVIDER
    "auto"
    CACHE STRING "TLS(Transport Layer Security) provider for trantor. Valid options are
 'none', 'openssl', 'botan-3', 'auto'."
)

# Version
set(TRANTOR_MAJOR_VERSION 1)
set(TRANTOR_MINOR_VERSION 5)
set(TRANTOR_PATCH_VERSION 17)
set(TRANTOR_VERSION ${TRANTOR_MAJOR_VERSION}.${TRANTOR_MINOR_VERSION}.${TRANTOR_PATCH_VERSION})
if(BUILD_SHARED_LIBS)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES VERSION
               ${TRANTOR_VERSION}
               SOVERSION
               ${TRANTOR_MAJOR_VERSION}
  )
endif()

# CMake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules/)

# Common settings
include(cmake/SetBuildOutput.cmake)
include(cmake/SetSystemAndCompiler.cmake)
include(cmake/SetThreads.cmake)
include(cmake/TestAtomic.cmake)

# Sources and headers
set(TRANTOR_SOURCES
    # cmake-format: sortable
    trantor/net/Channel.cc
    trantor/net/EventLoop.cc
    trantor/net/EventLoopThread.cc
    trantor/net/EventLoopThreadPool.cc
    trantor/net/InetAddress.cc
    trantor/net/inner/Acceptor.cc
    trantor/net/inner/AsyncStreamBufferNode.cc
    trantor/net/inner/Connector.cc
    trantor/net/inner/MemBufferNode.cc
    trantor/net/inner/Poller.cc
    trantor/net/inner/poller/EpollPoller.cc
    trantor/net/inner/poller/KQueue.cc
    trantor/net/inner/poller/PollPoller.cc
    trantor/net/inner/Socket.cc
    trantor/net/inner/StreamBufferNode.cc
    trantor/net/inner/TcpConnectionImpl.cc
    trantor/net/inner/Timer.cc
    trantor/net/inner/TimerQueue.cc
    trantor/net/TcpClient.cc
    trantor/net/TcpServer.cc
    trantor/utils/AsyncFileLogger.cc
    trantor/utils/ConcurrentTaskQueue.cc
    trantor/utils/Date.cc
    trantor/utils/Logger.cc
    trantor/utils/LogStream.cc
    trantor/utils/MsgBuffer.cc
    trantor/utils/SerialTaskQueue.cc
    trantor/utils/TimingWheel.cc
    trantor/utils/Utilities.cc
)
set(PRIVATE_HEADERS
    # cmake-format: sortable
    trantor/net/inner/Acceptor.h
    trantor/net/inner/Connector.h
    trantor/net/inner/Poller.h
    trantor/net/inner/poller/EpollPoller.h
    trantor/net/inner/poller/KQueue.h
    trantor/net/inner/poller/PollPoller.h
    trantor/net/inner/Socket.h
    trantor/net/inner/TcpConnectionImpl.h
    trantor/net/inner/Timer.h
    trantor/net/inner/TimerQueue.h
)

if(WIN32)
  list(
    APPEND
    TRANTOR_SOURCES
    # cmake-format: sortable
    third_party/wepoll/Wepoll.c
    trantor/utils/WindowsSupport.cc
    trantor/net/inner/FileBufferNodeWin.cc
  )
  list(
    APPEND
    PRIVATE_HEADERS
    # cmake-format: sortable
    third_party/wepoll/Wepoll.h
    trantor/utils/WindowsSupport.h
  )
else()
  list(APPEND TRANTOR_SOURCES trantor/net/inner/FileBufferNodeUnix.cc)
endif()

set(PUBLIC_NET_HEADERS
    # cmake-format: sortable
    trantor/net/AsyncStream.h
    trantor/net/callbacks.h
    trantor/net/Certificate.h
    trantor/net/Channel.h
    trantor/net/EventLoop.h
    trantor/net/EventLoopThread.h
    trantor/net/EventLoopThreadPool.h
    trantor/net/InetAddress.h
    trantor/net/Resolver.h
    trantor/net/TcpClient.h
    trantor/net/TcpConnection.h
    trantor/net/TcpServer.h
    trantor/net/TLSPolicy.h
)

set(PUBLIC_UTILS_HEADERS
    # cmake-format: sortable
    trantor/utils/AsyncFileLogger.h
    trantor/utils/ConcurrentTaskQueue.h
    trantor/utils/Date.h
    trantor/utils/Funcs.h
    trantor/utils/LockFreeQueue.h
    trantor/utils/Logger.h
    trantor/utils/LogStream.h
    trantor/utils/MsgBuffer.h
    trantor/utils/NonCopyable.h
    trantor/utils/ObjectPool.h
    trantor/utils/SerialTaskQueue.h
    trantor/utils/TaskQueue.h
    trantor/utils/TimingWheel.h
    trantor/utils/Utilities.h
)

# Build Dependencies
include(cmake/SetInstallDirs.cmake)
include(cmake/BuildDependencies.cmake)

# Set options
include(cmake/SetCAres.cmake)
include(cmake/SetSpdlog.cmake)
include(cmake/SetTlsProvider.cmake)
include(cmake/SetTesting.cmake)

# Export header
include(cmake/SetExportHeader.cmake)

# Group sources
source_group("Public API" FILES ${PUBLIC_NET_HEADERS} ${PUBLIC_UTILS_HEADERS} ${TRANTOR_EXPORT_HEADER})
source_group("Private Headers" FILES ${PRIVATE_HEADERS})

# Target sources
target_sources(
  ${PROJECT_NAME}
  PRIVATE ${TRANTOR_SOURCES}
          ${PRIVATE_HEADERS}
          ${PUBLIC_NET_HEADERS}
          ${PUBLIC_UTILS_HEADERS}
          ${TRANTOR_EXPORT_HEADER}
)

# Target include directories
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}> $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/exports>
  PRIVATE ${PROJECT_SOURCE_DIR}
          ${PROJECT_SOURCE_DIR}/trantor/utils
          ${PROJECT_SOURCE_DIR}/trantor/net
          ${PROJECT_SOURCE_DIR}/trantor/net/inner
          $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party/wepoll>
)

# Set additional
include(cmake/SetDoxygen.cmake)
include(cmake/SetPackageConfig.cmake)
include(cmake/SetInstall.cmake)
