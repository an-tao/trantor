#[[
 Get download url from github, provided by github release,
 if no release, use latest tag,
 if provided lib_name_BUILD_VERSION variable, use provided version
 if provided lib_name_VERSION_MAJOR variable, use latest version of the major version
#]]
macro(get_github_latest_release_url org_name lib_name)
  if(${lib_name}_BUILD_VERSION)
    set(${lib_name}_DOWNLOAD_URL
        "https://github.com/${org_name}/${lib_name}/archive/refs/tags/${lib_name}_BUILD_VERSION.zip"
    )
  else()
    find_program(CURL_EXECUTABLE curl)
    if(NOT CURL_EXECUTABLE)
      message(FATAL_ERROR "⛔CURL not found")
    endif()

    # Get latest release, if not provided by lib_name_VERSION_MAJOR
    if(NOT DEFINED (${lib_name}_VERSION_MAJOR))
      message(STATUS "Getting latest release from github.com")
      execute_process(
        COMMAND curl -v --connect-timeout 15 -L https://api.github.com/repos/${org_name}/${lib_name}/releases/latest
                ERROR_VARIABLE CURL_ERROR RESULT_VARIABLE CURL_RETURN_CODE OUTPUT_VARIABLE latest_json
      )
      if(NOT
         CURL_RETURN_CODE
         EQUAL
         0
      )
        message(FATAL_ERROR "⛔Failed to fetch latest release from github.com")
      endif()

      # Get latest release, or latest tag
      string(
        REGEX MATCH
              "\"tag_name\": \"[a-zA-Z]*-*[0-9]+[\\.|-|_][0-9]+[\\.|-|_][0-9]+"
              latest_tag
              "${latest_json}"
      )

    endif()

    if("${latest_tag}" STREQUAL "")
      # Get all tags
      message(STATUS "Getting all tags from github.com")
      execute_process(
        COMMAND curl -v --connect-timeout 15 -L https://api.github.com/repos/${org_name}/${lib_name}/tags
                ERROR_VARIABLE CURL_ERROR RESULT_VARIABLE CURL_RETURN_CODE OUTPUT_VARIABLE tags_json
      )
      if(NOT
         CURL_RETURN_CODE
         EQUAL
         0
      )
        message(FATAL_ERROR "⛔Failed to fetch tags from github.com: ${CURL_ERROR}")
      endif()

      # latest release is null, get latest tag
      if(DEFINED ${lib_name}_VERSION_MAJOR)
        string(
          REGEX MATCH
                "\"name\": \"[a-zA-Z]*-*[${${lib_name}_VERSION_MAJOR}]+[\\.|-|_][0-9]+[\\.|-|_][0-9]+"
                matched_tag
                "${tags_json}"
        )
      else()
        string(
          REGEX MATCH
                "\"name\": \"[a-zA-Z]*-*[0-9]+[\\.|-|_][0-9]+[\\.|-|_][0-9]+"
                matched_tag
                "${tags_json}"
        )
      endif()

      string(
        SUBSTRING "${matched_tag}"
                  9
                  -1
                  final_tag_val
      )
    else()
      string(
        SUBSTRING "${latest_tag}"
                  13
                  -1
                  final_tag_val
      )
    endif()

    # Final check tag value
    if("${final_tag_val}" STREQUAL "")
      message(FATAL_ERROR "⛔${org_name}/${lib_name}: Regex Got empty tag")
    else()
      set(${lib_name}_BUILD_VERSION
          "${final_tag_val}"
          CACHE STRING "final tag in ${org_name}/${lib_name}"
      )
      message(STATUS "Final tag: ${final_tag_val}")
      message(STATUS "🚩Passing ${lib_name}_BUILD_VERSION to change version")
    endif()

    set(${lib_name}_DOWNLOAD_URL
        "https://github.com/${org_name}/${lib_name}/archive/refs/tags/${final_tag_val}.zip"
        CACHE STRING "Download URL"
    )
  endif()
endmacro()

# copy dll/libs for Windows, to solve the test/unittests running 0x000135 error
macro(copy_files_for_win SourcePath EXT)
  if(WIN32)
    if(NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE "Release")
    endif()

    file(GLOB_RECURSE files ${SourcePath}/*.${EXT})
    foreach(f ${files})
      message(STATUS "copying ${f} to ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")
      file(COPY ${f} DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    endforeach()
  endif()
endmacro()

# Set FetchContent
macro(set_fetch_content)
  message(STATUS "Setting FetchContent: ${FETCH_BUILD_MISSING_DEPS}")

  # CMake >= 3.11
  if(${CMAKE_VERSION} VERSION_LESS "3.11")
    message(FATAL_ERROR "⛔CMake >= 3.11 required")
  endif()

  # if cmake < 3.14, let FetchContent_MakeAvailable working also
  if(${CMAKE_VERSION} VERSION_LESS "3.14")
    macro(FetchContent_MakeAvailable NAME)
      FetchContent_GetProperties(${NAME})
      if(NOT ${NAME}_POPULATED)
        FetchContent_Populate(${NAME})
        add_subdirectory(${${NAME}_SOURCE_DIR} ${${NAME}_BINARY_DIR})
      endif()
    endmacro()
  endif()

  include(FetchContent)
  # show progress
  set(FETCHCONTENT_QUIET OFF)
  set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

  # disable CMP0135 warning
  if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24")
    set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
    set(DOWNLOAD_EXTRACT_TIMESTAMP ON)
  endif()
endmacro()

# ######################################################################################################################
# Set build output path, this is mainly for Windows, to solve the test/unittests running 0x000135 error
if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# Using CMAKE_TOOLCHAIN_FILE disable build missing dependencies
if(CMAKE_TOOLCHAIN_FILE OR ENV{CMAKE_TOOLCHAIN_FILE})
  set(FETCH_BUILD_MISSING_DEPS OFF)
  message(STATUS "⚠️Using CMAKE_TOOLCHAIN_FILE, it disable FETCH_BUILD_MISSING_DEPS")
else()
  set(FETCH_BUILD_MISSING_DEPS ON)
  set_fetch_content()
endif()
