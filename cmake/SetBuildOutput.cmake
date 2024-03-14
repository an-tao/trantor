# Set build output path, this is mainly for Windows, to solve the test/unittests running 0x000135 problems
macro(set_standard_build_output Tgt)
  if(WIN32)
    set_target_properties(${Tgt} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set_target_properties(${Tgt} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set_target_properties(${Tgt} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  endif()
endmacro()

# copy dll, this is mainly for Windows, to solve the test/unittests running 0x000135 problems
macro(copy_dlls_to_standard_build_output SPath)
  if(WIN32)
    if(NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE "Release")
    endif()

    file(GLOB_RECURSE dlls ${SPath}/*.dll)
    foreach(dll ${dlls})
      message(STATUS "copying ${dll} to ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
      file(COPY ${dll} DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    endforeach()
  endif()
endmacro()
