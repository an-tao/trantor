cmake_minimum_required(VERSION 3.11)
project(
  trantor
  DESCRIPTION "Non-blocking I/O cross-platform TCP network library, using C++14"
  HOMEPAGE_URL "https://github.com/an-tao/trantor"
)
add_library(${PROJECT_NAME})

# Version
set(TRANTOR_MAJOR_VERSION 1)
set(TRANTOR_MINOR_VERSION 5)
set(TRANTOR_PATCH_VERSION 20)
set(TRANTOR_VERSION ${TRANTOR_MAJOR_VERSION}.${TRANTOR_MINOR_VERSION}.${TRANTOR_PATCH_VERSION})

# C++ standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 14)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

# Old Build options
#[[
option(BUILD_C-ARES "Build C-ARES" ON)                          Replaced by TRANTOR_USE_C-ARES
option(USE_SPDLOG "Allow using the spdlog logging library" OFF) Replaced by TRANTOR_USE_SPDLOG
option(TRANTOR_USE_TLS "TLS provider for trantor." "")          Replaced by TRANTOR_TLS_PROVIDER
#]]

# Build options
option(BUILD_SHARED_LIBS "Build ${PROJECT_NAME} as a shared lib" ON)
option(BUILD_TESTING "Build tests" OFF)
option(BUILD_DOC "Build Doxygen documentation" OFF)
option(TRANTOR_USE_SPDLOG "Using the spdlog logging library" ON)
option(TRANTOR_USE_C-ARES "Using C-ARES" ON)
option(TRANTOR_TLS_PROVIDER
       "TLS(Transport Layer Security) provider. Valid options are 'OFF', 'ON', 'none', 'openssl', 'botan-3', 'auto'."
       ON
)
option(FETCH_BUILD_MISSING_DEPS "Fetch/build/install missing dependencies, CMake version >=3.11 required" ON)
option(UPDATE_CONAN_FILE
       "Update conan file and install conan packages, using CMAKE_TOOLCHAIN_FILE, CMAKE verion >=3.15 required" OFF
)

if(BUILD_SHARED_LIBS)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES VERSION
               ${TRANTOR_VERSION}
               SOVERSION
               ${TRANTOR_MAJOR_VERSION}
  )
endif()

# Change TRANTOR_TLS_PROVIDER ON/OFF to String
if(TRANTOR_TLS_PROVIDER STREQUAL OFF)
  set(TRANTOR_TLS_PROVIDER
      "none"
      CACHE STRING "" FORCE
  )
elseif(TRANTOR_TLS_PROVIDER STREQUAL ON)
  set(TRANTOR_TLS_PROVIDER
      "botan-3"
      CACHE STRING "" FORCE
  )
else()
  # keep value provided by -DTRANTOR_TLS_PROVIDER, such as -DTRANTOR_TLS_PROVIDER=openssl
endif()

# Sources and headers
set(public_utils_headers
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

set(public_net_headers
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

set(private_headers
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

if(WIN32)
  list(
    APPEND
    private_headers
    # cmake-format: sortable
    third_party/wepoll/Wepoll.h
    trantor/utils/WindowsSupport.h
  )
  list(
    APPEND
    TRANTOR_SOURCES
    # cmake-format: sortable
    third_party/wepoll/Wepoll.c
    trantor/utils/WindowsSupport.cc
    trantor/net/inner/FileBufferNodeWin.cc
  )
else()
  list(APPEND TRANTOR_SOURCES trantor/net/inner/FileBufferNodeUnix.cc)
endif()

# Export header
include(cmake/SetExportHeader.cmake)

# include directories
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

# CMake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules/)

# Common settings
include(cmake/SetSystemAndCompiler.cmake)

# Update conan/vcpkg dependencies, and install them, remove CMakeCache.txt and Configure CMake Without UPDATE_CONAN_FILE
# OR INSTALL_VPKG_DEPS again
if(UPDATE_CONAN_FILE)
  include(cmake/SetConan.cmake)
endif()

# Set options
if(NOT UPDATE_CONAN_FILE AND NOT INSTALL_DEPS_VCPKG)
  include(cmake/SetDepsOptions.cmake)
  include(cmake/SetSpdlog.cmake)
  include(cmake/SetCAres.cmake)
  include(cmake/SetTlsProvider.cmake)
  include(cmake/SetTesting.cmake)
  include(cmake/SetDoxygen.cmake)
endif()

# Target sources
target_sources(
  ${PROJECT_NAME}
  PRIVATE ${TRANTOR_SOURCES}
          ${TRANTOR_EXPORT_HEADER}
          ${public_net_headers}
          ${public_utils_headers}
          ${private_headers}
)

# Group sources
source_group("Public API" FILES ${TRANTOR_EXPORT_HEADER} ${public_net_headers} ${public_utils_headers})
source_group("Private Headers" FILES ${private_headers})

# Set additional
include(cmake/SetInstall.cmake)
include(cmake/SetPackageConfig.cmake)
