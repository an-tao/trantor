cmake_minimum_required(VERSION 3.2)
project(trantor)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    EXEC_PROGRAM (gcc ARGS "--version | grep '^gcc'|awk '{print $3}' | sed s'/)//g' | sed s'/-.*//g'" OUTPUT_VARIABLE version)
    MESSAGE(STATUS "This is gcc version:: " ${version})
    if(version LESS 4.9.0)
        MESSAGE(STATUS "gcc is too old")
        stop()
    endif()
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
    CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
    if(COMPILER_SUPPORTS_CXX17)
        message(STATUS "support c++17")
        set(CMAKE_CXX_STD_FLAGS c++17)
        set(DEFS "USE_STD_ANY")
    elseif(COMPILER_SUPPORTS_CXX14)
        message(STATUS "support c++14")
        set(CMAKE_CXX_STD_FLAGS c++14)
    elseif(COMPILER_SUPPORTS_CXX11)
        message(STATUS "support c++11")
        set(CMAKE_CXX_STD_FLAGS c++11)
    endif()
else()
    set(CMAKE_CXX_STD_FLAGS c++11)
endif()

find_package(Boost)
if(Boost_FOUND)
    SET(DEFS "USE_BOOST;${DEFS}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -std=${CMAKE_CXX_STD_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -std=${CMAKE_CXX_STD_FLAGS}")

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

AUX_SOURCE_DIRECTORY(trantor/utils DIR_SRC)
AUX_SOURCE_DIRECTORY(trantor/net DIR_SRC)
AUX_SOURCE_DIRECTORY(trantor/net/inner DIR_SRC)
find_package (OpenSSL)
if(OpenSSL_FOUND)
    #add_definitions(-DUSE_OPENSSL)
    set(DEFS "USE_OPENSSL;${DEFS}")
    include_directories(${OPENSSL_INCLUDE_DIR})
    AUX_SOURCE_DIRECTORY(trantor/net/ssl DIR_SRC)
endif()

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    ADD_LIBRARY(trantor ${DIR_SRC} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/EpollPoller.cc)
    MESSAGE(STATUS "current platform: Linux ")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(STATUS "current platform: Windows")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    MESSAGE(STATUS "current platform: FreeBSD")
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    ADD_LIBRARY(trantor ${DIR_SRC} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/PollPoller.cc)
    MESSAGE(STATUS "current platform: MacOS")
ELSE ()
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")


SET(CONFIG_HEADER "${PROJECT_SOURCE_DIR}/config.h")
file(WRITE "${CONFIG_HEADER}" "#pragma once\n")
foreach(loop_var ${DEFS})
    file(APPEND "${CONFIG_HEADER}" "#define ${loop_var}\n")
endforeach(loop_var)
file(APPEND "${CONFIG_HEADER}" "#ifdef USE_STD_ANY\n")

file(APPEND "${CONFIG_HEADER}" "#include <any>\n")
file(APPEND "${CONFIG_HEADER}" "using std::any;\n")
file(APPEND "${CONFIG_HEADER}" "using std::any_cast;\n")

file(APPEND "${CONFIG_HEADER}" "#else \n")
file(APPEND "${CONFIG_HEADER}" "#ifdef USE_BOOST\n")

file(APPEND "${CONFIG_HEADER}" "#include <boost/any.hpp>\n")
file(APPEND "${CONFIG_HEADER}" "using boost::any;\n")
file(APPEND "${CONFIG_HEADER}" "using boost::any_cast;\n")

file(APPEND "${CONFIG_HEADER}" "#else\n")
file(APPEND "${CONFIG_HEADER}" "#error,must use c++17 or boost to support any\n")
file(APPEND "${CONFIG_HEADER}" "#endif\n")
file(APPEND "${CONFIG_HEADER}" "#endif\n")

EXEC_PROGRAM(${PROJECT_SOURCE_DIR}/update_config.sh ARGS "${CONFIG_HEADER} ${PROJECT_SOURCE_DIR}/trantor/utils/config.h")

ADD_SUBDIRECTORY(trantor/tests)
find_package(GTest)
if(GTest_FOUND)
    include_directories(GTEST_INCLUDE_DIR)
    #link_directories(GTEST_LIBRARY_DIR)
    ADD_SUBDIRECTORY(trantor/unittests)
endif()

#install
SET(CMAKE_INSTALL_PREFIX /usr/local)

install(TARGETS trantor DESTINATION lib)

install(DIRECTORY trantor/utils/ DESTINATION include/trantor/utils
        FILES_MATCHING PATTERN "*.h")

file(GLOB trantor_net_headers "${CMAKE_CURRENT_SOURCE_DIR}/trantor/net/*.h")
install(FILES ${trantor_net_headers} DESTINATION include/trantor/net)

