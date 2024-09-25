# Get latest verion from conan.io/center
macro(get_conan_latest_build_version lib_name)
  find_program(CURL_EXECUTABLE curl)
  if(NOT CURL_EXECUTABLE)
    message(FATAL_ERROR "⛔CURL not found")
  endif()

  # Get latest build info
  execute_process(
    COMMAND curl -v --connect-timeout 15 -L https://conan.io/center/recipes/${lib_name} ERROR_VARIABLE CURL_ERROR
            RESULT_VARIABLE CURL_RETURN_CODE OUTPUT_VARIABLE html_text
  )
  if(NOT
     CURL_RETURN_CODE
     EQUAL
     0
  )
    message(FATAL_ERROR "⛔Failed to get latest build info from conan.io: ${CURL_ERROR}")
  endif()

  # Get latest build version
  string(
    REGEX MATCH
          "\"version\":\"[a-zA-Z]*-*[0-9]+[\\.|-|_][0-9]+[\\.|-|_][0-9]+"
          latest_build
          "${html_text}"
  )

  if("${latest_build}" STREQUAL "")
    message(FATAL_ERROR "⛔${lib_name}: Regex Got empty build")
  else()
    string(
      SUBSTRING "${latest_build}"
                11
                -1
                latest_build_val
    )
    set(${lib_name}_BUILD_VERSION "${latest_build_val}")
    message(STATUS "${lib_name} latest build: ${latest_build_val}")
  endif()
endmacro()

# ######################################################################################################################
message(STATUS "Updating conanfile.txt")

if(EXISTS conanfile.json)
  file(REMOVE conanfile.json)
endif()

# add start text
set(conanfile_txt "[requires]\n")
set(cppstd "--settings=compiler.cppstd=20")
# add dependencies
if(TRANTOR_USE_SPDLOG)
  get_conan_latest_build_version(spdlog)
  string(APPEND conanfile_txt "spdlog/${spdlog_BUILD_VERSION}\n")
endif()
if(TRANTOR_USE_C-ARES)
  get_conan_latest_build_version(c-ares)
  string(APPEND conanfile_txt "c-ares/${c-ares_BUILD_VERSION}\n")
endif()
if(TRANTOR_TLS_PROVIDER STREQUAL "auto")
  get_conan_latest_build_version(openssl)
  string(APPEND conanfile_txt "openssl/${openssl_BUILD_VERSION}\n")
endif()
if(TRANTOR_TLS_PROVIDER STREQUAL "openssl")
  get_conan_latest_build_version(openssl)
  string(APPEND conanfile_txt "openssl/${openssl_BUILD_VERSION}\n")
endif()
if(TRANTOR_TLS_PROVIDER STREQUAL "botan-3")
  get_conan_latest_build_version(botan)
  string(APPEND conanfile_txt "botan/${botan_BUILD_VERSION}\n")
  set(cppstd "--settings=compiler.cppstd=20")
endif()
if(BUILD_TESTING)
  get_conan_latest_build_version(gtest)
  string(APPEND conanfile_txt "gtest/${gtest_BUILD_VERSION}\n")
endif()

# add end text
string(
  APPEND
  conanfile_txt
  "\

[generators]
CMakeDeps
CMakeToolchain

[options]

[imports]
"
)

file(WRITE conanfile.txt ${conanfile_txt})

# Install deps
message(STATUS "Installing conan packages")

find_program(CONAN_EXECUTABLE conan)
if(NOT CONAN_EXECUTABLE)
  message(FATAL_ERROR "⛔conan not found")
endif()

# build_type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Install
execute_process(
  COMMAND conan install . --output-folder=build --build=missing --settings=build_type=${CMAKE_BUILD_TYPE} ${cppstd}
          RESULT_VARIABLE CONAN_RETURN_CODE
)
if(NOT
   CONAN_RETURN_CODE
   EQUAL
   0
)
  message(FATAL_ERROR "⛔Failed to install conan packages")
endif()
