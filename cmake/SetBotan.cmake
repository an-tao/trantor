# build and install botan
macro(build_install_botan)

  # botan build need python
  find_package(PythonInterp REQUIRED)

  if(NOT Botan_DOWNLOAD_URL)
    get_github_latest_release_url("randombit" "Botan")
  endif()

  # [[ VS 17.11 the STL PR: microsoft/STL#4633 on April 27, 2024 Botan 3.5.0 or below, will build error ]]
  find_program(MSBUILD_EXECUTABLE msbuild)
  if(MSBUILD_EXECUTABLE)
    execute_process(COMMAND msbuild -version OUTPUT_VARIABLE msbuild_version OUTPUT_STRIP_TRAILING_WHITESPACE)
    message(STATUS "msbuild version: ${msbuild_version}")

    if(msbuild_version VERSION_GREATER "17.11" AND Botan_BUILD_VERSION VERSION_LESS_EQUAL "3.5.0")
      message(STATUS "⚠️After msbuild version: ${msbuild_version}, Botan must greater than 3.5.0, using master branch")
      FetchContent_Declare(Botan URL https://github.com/randombit/botan/archive/refs/heads/master.zip)
    else()
      FetchContent_Declare(Botan URL ${Botan_DOWNLOAD_URL})
    endif()

  else()
    FetchContent_Declare(Botan URL ${Botan_DOWNLOAD_URL})
  endif()

  FetchContent_GetProperties(Botan)

  if(NOT Botan_POPULATED)
    FetchContent_Populate(Botan)

    get_lowcase_compiler_id()

    # configure build and install
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} ${botan_SOURCE_DIR}/configure.py --prefix=${BOTAN_ROOT_DIR}
              --cc=${cmake_cxx_compiler_id} --disable-deprecated-features --with-pkg-config --build-target=shared
      WORKING_DIRECTORY ${botan_BINARY_DIR}
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
      execute_process(COMMAND nmake WORKING_DIRECTORY ${botan_BINARY_DIR})
      execute_process(COMMAND nmake install WORKING_DIRECTORY ${botan_BINARY_DIR})
    else()
      execute_process(COMMAND make -j4 WORKING_DIRECTORY ${botan_BINARY_DIR})
      execute_process(COMMAND make install WORKING_DIRECTORY ${botan_BINARY_DIR})
    endif()

  endif()
endmacro()

# Set botan
macro(set_botan)

  target_compile_definitions(${PROJECT_NAME} PRIVATE TRANTOR_TLS_PROVIDER="Botan${Botan_VERSION_MAJOR}")
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_BOTAN)
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_BOTAN${Botan_VERSION_MAJOR})

  target_include_directories(${PROJECT_NAME} PRIVATE ${Botan_INCLUDE_DIRS})

  # conan target is different, always lower case
  if(TARGET botan::botan)
    target_link_libraries(${PROJECT_NAME} PRIVATE botan::botan)
    target_compile_features(botan::botan INTERFACE cxx_std_20)
    # std::max、std::min error C2589: “(”:“::” #include <windows.h> #include <algorithm>
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
  endif()
  if(TARGET Botan::Botan)
    target_link_libraries(${PROJECT_NAME} PRIVATE Botan::Botan)
  endif()

  list(
    APPEND
    TRANTOR_SOURCES
    trantor/net/inner/tlsprovider/BotanTLSProvider.cc
    trantor/utils/crypto/botan.cc
  )

  # copy dll/libs for Windows, to solve the test/unittests running 0x000135 error
  copy_files_for_win(${BOTAN_ROOT_DIR}/bin "dll")
  copy_files_for_win(${BOTAN_ROOT_DIR}/lib "lib")

endmacro()

# Get lowcase compiler id
macro(get_lowcase_compiler_id)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(cmake_cxx_compiler_id "gcc")
  else()
    string(TOLOWER ${CMAKE_CXX_COMPILER_ID} cmake_cxx_compiler_id)
  endif()
endmacro()

# ######################################################################################################################
message(STATUS "Trantor using SSL library: botan")

if(NOT (DEFINED Botan_VERSION_MAJOR))
  message(STATUS "⛔Must specify Botan_VERSION_MAJOR")
endif()

# Set BOTAN_ROOT_DIR, to let find_package could search from it
if(WIN32)
  set(BOTAN_ROOT_DIR
      C:/Botan
      CACHE PATH "Let find_package use the directory to find" FORCE
  )
else()
  set(BOTAN_ROOT_DIR
      ${CMAKE_INSTALL_PREFIX}/Botan
      CACHE PATH "Let find_package use the directory to find" FORCE
  )
endif()

find_package(Botan ${Botan_VERSION_MAJOR})

if(NOT Botan_FOUND)
  if(FETCH_BUILD_MISSING_DEPS)
    message(STATUS "⚠️Botan${Botan_VERSION_MAJOR} not found, Building it with FetchContent...")
    unset(Botan_FOUND)

    build_install_botan()
    find_package(Botan ${Botan_VERSION_MAJOR} REQUIRED)

  else()
    message(
      FATAL_ERROR "⛔Botan${Botan_VERSION_MAJOR} not found, please install it or set FETCH_BUILD_MISSING_DEPS to ON"
    )
  endif()
endif()

set_botan()
