cmake_minimum_required(VERSION 3.2)
project(trantor)

EXEC_PROGRAM (gcc ARGS "--version | grep '^gcc'|awk '{print $3}' | sed s'/)//g'" OUTPUT_VARIABLE version)
MESSAGE(STATUS "This is gcc version:: " ${version})
if(version LESS 4.7.0)
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
    add_definitions(-DUSE_STD_ANY)
    MESSAGE(STATUS "c++17")

endif()

find_package(Boost)
if(Boost_FOUND)
    add_definitions(-DUSE_BOOST)
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
if (MAKETEST STREQUAL YES)
ADD_SUBDIRECTORY(trantor/tests)
endif ()
# lib directories
LINK_DIRECTORIES(
        ${PROJECT_BINARY_DIR}/lib

        /usr/local/lib
        /use/lib
)

AUX_SOURCE_DIRECTORY(trantor/utils DIR_SRC)
AUX_SOURCE_DIRECTORY(trantor/net DIR_SRC)
AUX_SOURCE_DIRECTORY(trantor/net/inner DIR_SRC)
ADD_LIBRARY(trantor ${DIR_SRC})


SET(CMAKE_INSTALL_PREFIX /usr/local)

install(TARGETS trantor DESTINATION lib)

install(DIRECTORY trantor/utils/ DESTINATION include/trantor/utils
        FILES_MATCHING PATTERN "*.h")

file(GLOB trantor_net_headers "${CMAKE_CURRENT_SOURCE_DIR}/trantor/net/*.h")
install(FILES ${trantor_net_headers} DESTINATION include/trantor/net)

