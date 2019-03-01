cmake_minimum_required(VERSION 3.2)
project(trantor)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    EXEC_PROGRAM (gcc ARGS "--version | grep '^gcc'|awk '{print $3}' | sed s'/)//g' | sed s'/-.*//g'" OUTPUT_VARIABLE version)
    MESSAGE(STATUS "This is gcc version:: " ${version})
    if(version LESS 5.4.0)
        MESSAGE(STATUS "gcc is too old")
        stop()
    elseif(version LESS 7.1.0)
        set(CMAKE_CXX_STD_FLAGS c++14)
        MESSAGE(STATUS "c++14")
    else()
        set(CMAKE_CXX_STD_FLAGS c++17)
        set(DEFS "USE_STD_ANY")
        set(USE_STD_ANY 1)
        MESSAGE(STATUS "c++17")
    endif()
else()
    set(DEFS "USE_STD_ANY")
    set(USE_STD_ANY 1)
    set(CMAKE_CXX_STD_FLAGS c++17)
endif()

# include directories
INCLUDE_DIRECTORIES(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/trantor/utils
        ${PROJECT_SOURCE_DIR}/trantor/net
        ${PROJECT_SOURCE_DIR}/trantor/net/inner
        /usr/local/include
        /usr/include
)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -std=${CMAKE_CXX_STD_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -std=${CMAKE_CXX_STD_FLAGS}")


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
    link_libraries(${OPENSSL_LIBRARIES})
    AUX_SOURCE_DIRECTORY(trantor/net/ssl DIR_SRC)
endif()

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    ADD_LIBRARY(trantor ${DIR_SRC} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/EpollPoller.cc)
    MESSAGE(STATUS "current platform: Linux ")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(STATUS "current platform: Windows")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    ADD_LIBRARY(trantor ${DIR_SRC} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/KQueue.cc)
    MESSAGE(STATUS "current platform: FreeBSD")
ELSEIF(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    #ADD_LIBRARY(trantor ${DIR_SRC} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/PollPoller.cc)
    ADD_LIBRARY(trantor ${DIR_SRC} ${PROJECT_SOURCE_DIR}/trantor/net/inner/poller/KQueue.cc)
    MESSAGE(STATUS "current platform: MacOS")
ELSE ()
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")


SET(CONFIG_HEADER "${PROJECT_SOURCE_DIR}/config.h")
file(WRITE "${CONFIG_HEADER}" "#pragma once\n")
foreach(loop_var ${DEFS})
    file(APPEND "${CONFIG_HEADER}" "#define ${loop_var}\n")
endforeach(loop_var)
file(APPEND "${CONFIG_HEADER}" "\n")

if(DEFINED USE_STD_ANY)

file(APPEND "${CONFIG_HEADER}" "#include <any>\n")
file(APPEND "${CONFIG_HEADER}" "using std::any;\n")
file(APPEND "${CONFIG_HEADER}" "using std::any_cast;\n\n")
file(APPEND "${CONFIG_HEADER}" "#include <string_view>\n")
file(APPEND "${CONFIG_HEADER}" "typedef std::string_view string_view;\n\n")

else()

file(APPEND "${CONFIG_HEADER}" "#include <experimental/any>\n")
file(APPEND "${CONFIG_HEADER}" "using std::experimental::any;\n")
file(APPEND "${CONFIG_HEADER}" "using std::experimental::any_cast;\n\n")
file(APPEND "${CONFIG_HEADER}" "#include <experimental/string_view>\n")
file(APPEND "${CONFIG_HEADER}" "typedef std::experimental::basic_string_view<char> string_view;\n\n")

endif()

EXEC_PROGRAM(${PROJECT_SOURCE_DIR}/update_config.sh ARGS "${CONFIG_HEADER} ${PROJECT_SOURCE_DIR}/trantor/utils/config.h")

if (MAKETEST STREQUAL YES)
    ADD_SUBDIRECTORY(trantor/tests)
endif ()

#install
SET(CMAKE_INSTALL_PREFIX /usr/local)

install(TARGETS trantor DESTINATION lib)

install(DIRECTORY trantor/utils/ DESTINATION include/trantor/utils
        FILES_MATCHING PATTERN "*.h")

file(GLOB trantor_net_headers "${CMAKE_CURRENT_SOURCE_DIR}/trantor/net/*.h")
install(FILES ${trantor_net_headers} DESTINATION include/trantor/net)



