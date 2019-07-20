CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
PROJECT(trantor)

# include directories
INCLUDE_DIRECTORIES(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/trantor/utils
        ${PROJECT_SOURCE_DIR}/trantor/net
        ${PROJECT_SOURCE_DIR}/trantor/net/inner
        /usr/local/include
        /usr/include
)

# lib directories
LINK_DIRECTORIES(
        ${PROJECT_BINARY_DIR}/
        /usr/local/lib
        /usr/lib
)

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
    trantor/net/inner/TimerQueue.cc)

FIND_PACKAGE(OpenSSL)
IF(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    LINK_LIBRARIES(${OPENSSL_LIBRARIES})
    SET(trantor_sources ${trantor_sources} trantor/net/ssl/SSLConnection.cc)
ELSE()
    SET(trantor_sources ${trantor_sources} trantor/net/ssl/SSLConnectionSkipped.cc)
ENDIF()

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(trantor_sources ${trantor_sources} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/EpollPoller.cc)
    MESSAGE(STATUS "current platform: Linux ")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(FATAL_ERROR "Error: Currently does not support Windows")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    SET(trantor_sources ${trantor_sources} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/KQueue.cc)
    MESSAGE(STATUS "current platform: FreeBSD")
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    SET(trantor_sources ${trantor_sources} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/KQueue.cc)
    MESSAGE(STATUS "current platform: MacOS")
ELSE ()
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

ADD_LIBRARY(trantor ${trantor_sources})
SET_PROPERTY(TARGET trantor PROPERTY CXX_STANDARD 14)
SET_PROPERTY(TARGET trantor PROPERTY CXX_STANDARD_REQUIRED ON)
SET_PROPERTY(TARGET trantor PROPERTY CXX_EXTENSIONS OFF)

IF (MAKETEST STREQUAL YES)
    ADD_SUBDIRECTORY(trantor/tests)
ENDIF ()

INSTALL(TARGETS trantor DESTINATION lib)

INSTALL(FILES ${public_net_headers} DESTINATION include/trantor/net)
INSTALL(FILES ${public_utils_headers} DESTINATION include/trantor/utils)
