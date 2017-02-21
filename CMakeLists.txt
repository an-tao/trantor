cmake_minimum_required(VERSION 3.2)
project(trantor)
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
        -std=c++11
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
ADD_SUBDIRECTORY(trantor/tests)
ADD_SUBDIRECTORY(trantor/unittests)

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

