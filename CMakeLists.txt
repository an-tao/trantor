cmake_minimum_required(VERSION 3.5)
project(trantor)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")
# include directories
INCLUDE_DIRECTORIES(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/trantor/utils
  ${PROJECT_SOURCE_DIR}/trantor/net

  /usr/local/include
  /usr/include
)
ADD_SUBDIRECTORY(trantor/tests)
# lib directories
LINK_DIRECTORIES(
  ${PROJECT_BINARY_DIR}/lib

  /usr/local/lib
  /use/lib
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

AUX_SOURCE_DIRECTORY(trantor/utils DIR_SRC)
AUX_SOURCE_DIRECTORY(trantor/net DIR_SRC)
ADD_LIBRARY(trantor ${DIR_SRC})
target_link_libraries(trantor pthread rt)

