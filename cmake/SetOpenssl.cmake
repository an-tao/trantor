# build and install openssl
macro(build_install_openssl)

  # openssl build need perl
  find_program(PERL_EXECUTABLE perl)
  if(NOT PERL_EXECUTABLE)
    message(FATAL_ERROR "⛔Perl not found")
  endif()

  if(NOT OpenSSL_DOWNLOAD_URL)
    get_github_latest_release_url("openssl" "OpenSSL")
  endif()
  FetchContent_Declare(openssl URL ${OpenSSL_DOWNLOAD_URL})
  FetchContent_GetProperties(OpenSSL)

  if(NOT OpenSSL_POPULATED)
    FetchContent_Populate(OpenSSL)

    # ⚠️ remove "no-asm" flags to improve openssl performance
    execute_process(
      COMMAND ${PERL_EXECUTABLE} ${openssl_SOURCE_DIR}/Configure no-stdio no-engine no-apps no-asm
              --prefix=${OPENSSL_ROOT_DIR} WORKING_DIRECTORY ${openssl_BINARY_DIR}
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
      execute_process(COMMAND nmake WORKING_DIRECTORY ${openssl_BINARY_DIR})
      execute_process(COMMAND nmake install WORKING_DIRECTORY ${openssl_BINARY_DIR})
    else()
      execute_process(COMMAND make -j4 WORKING_DIRECTORY ${openssl_BINARY_DIR})
      execute_process(COMMAND make install WORKING_DIRECTORY ${openssl_BINARY_DIR})
    endif() # compiler id

  endif()

endmacro()

# Set openssl
macro(set_openssl)

  target_compile_definitions(${PROJECT_NAME} PRIVATE TRANTOR_TLS_PROVIDER="OpenSSL")
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_OPENSSL)

  target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
  if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Crypt32 Secur32)
  endif()

  list(
    APPEND
    TRANTOR_SOURCES
    trantor/net/inner/tlsprovider/OpenSSLProvider.cc
    trantor/utils/crypto/openssl.cc
  )

  # copy dll/libs for Windows, to solve the test/unittests running 0x000135 error
  copy_files_for_win(${OPENSSL_ROOT_DIR}/bin "dll")
  copy_files_for_win(${OPENSSL_ROOT_DIR}/lib "lib")

endmacro()

# ######################################################################################################################
message(STATUS "Trantor using SSL library: openssl")
# Set OPENSSL_ROOT_DIR, to let find_package could search from the build directory
if(WIN32)
  set(OPENSSL_ROOT_DIR
      C:/OpenSSL
      CACHE PATH "Let find_package use the directory to find" FORCE
  )
else()
  set(OPENSSL_ROOT_DIR
      ${CMAKE_INSTALL_PREFIX}/OpenSSL
      CACHE PATH "Let find_package use the directory to find" FORCE
  )
endif()

find_package(OpenSSL)

if(NOT OpenSSL_FOUND)
  if(FETCH_BUILD_MISSING_DEPS)
    message(STATUS "⚠️OpenSSL not found, Building it with FetchContent...")
    unset(OpenSSL_FOUND)

    build_install_openssl()
    find_package(OpenSSL REQUIRED)

  else()
    message(FATAL_ERROR "⛔OpenSSL not found, please install it or set TRANTOR_DEPENDENCIES_PROVIDER to source")
  endif()
endif()

set_openssl()
