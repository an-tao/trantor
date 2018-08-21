cmake_minimum_required(VERSION 3.2)
project(trantor)
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    EXEC_PROGRAM (gcc ARGS "--version | grep '^gcc'|awk '{print $3}' | sed s'/)//g' | sed s'/-.*//g'" OUTPUT_VARIABLE version)
    MESSAGE(STATUS "This is gcc version:: " ${version})
    if(version LESS 4.9.0)
        MESSAGE(STATUS "gcc is too old")
        stop()
    elseif (version LESS 6.1.0)
        MESSAGE(STATUS "c++11")
        set(CMAKE_CXX_STD_FLAGS c++11)
    elseif(version LESS 7.1.0)
        set(CMAKE_CXX_STD_FLAGS c++14)
        MESSAGE(STATUS "c++14")
    else()
        set(CMAKE_CXX_STD_FLAGS c++17)
        SET(DEFS "USE_STD_ANY")
        MESSAGE(STATUS "c++17")

    endif()
else()
    set(CMAKE_CXX_STD_FLAGS c++11)
endif()
find_package(Boost)
if(Boost_FOUND)
    SET(DEFS "USE_BOOST;${DEFS}")
    include_directories(${Boost_INCLUDE_DIRS})
endif()



set(CXX_FLAGS
        -g
        # -DVALGRIND
        # -DMUDUO_STD_STRING
        #-DCHECK_PTHREAD_RETURN_VALUE
        #-D_FILE_OFFSET_BITS=64
        -Wall
        #-Wextra
        #-Werror
        #-Wconversion
        #-Wno-unused-parameter
        #-Wold-style-cast
        #-Woverloaded-virtual
        #-Wpointer-arith
        #-Wshadow
        #-Wwrite-strings
        #-march=native
        # -MMD
        -std=${CMAKE_CXX_STD_FLAGS}
        #-rdynamic
        #-O2
        #-finline-limit=1000
        #-DNDEBUG
        )

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
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



