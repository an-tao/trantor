# build and install c-ares
macro(build_install_cares)
  if(NOT c-ares_DOWNLOAD_URL)
    get_github_latest_release_url("c-ares" "c-ares")
  endif()
  FetchContent_Declare(c-ares URL ${c-ares_DOWNLOAD_URL})
  # not using FetchContent_MakeAvailable to save compile time
  FetchContent_GetProperties(c-ares)

  if(NOT c-ares_POPULATED)
    FetchContent_Populate(c-ares)

    # configure build and install, c-ares provided makefiles already, using them to build is faster
    if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
      execute_process(COMMAND buildconf.bat WORKING_DIRECTORY ${c-ares_SOURCE_DIR})
      execute_process(COMMAND nmake -f Makefile.msvc WORKING_DIRECTORY ${c-ares_SOURCE_DIR})
      set(ENV{INSTALL_DIR} ${C-ARES_DIR})
      execute_process(COMMAND nmake -f Makefile.msvc install WORKING_DIRECTORY ${c-ares_SOURCE_DIR})
    else()
      execute_process(COMMAND ./buildconf WORKING_DIRECTORY ${c-ares_SOURCE_DIR})
      execute_process(COMMAND make WORKING_DIRECTORY ${c-ares_SOURCE_DIR})
      execute_process(COMMAND make --install WORKING_DIRECTORY ${c-ares_SOURCE_DIR})
    endif()

  endif()
endmacro()

# Set normal resolver
macro(set_normal_resolver)
  list(APPEND TRANTOR_SOURCES trantor/net/inner/NormalResolver.cc)
  list(APPEND private_headers trantor/net/inner/NormalResolver.h)
endmacro()

# Set cares resolver
macro(set_cares_resolver)
  target_link_libraries(${PROJECT_NAME} PRIVATE c-ares::cares)

  if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE resolv)
  elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Iphlpapi)
  endif()

  if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CARES_STATICLIB)
  endif()

  list(APPEND TRANTOR_SOURCES trantor/net/inner/AresResolver.cc)
  list(APPEND private_headers trantor/net/inner/AresResolver.h)

  # copy dll/libs for Windows, to solve the test/unittests running 0x000135 error
  copy_files_for_win(${C-ARES_DIR}/bin "dll")
  copy_files_for_win(${C-ARES_DIR}/lib "lib")
  copy_files_for_win(${C-ARES_DIR}/lib "dll")
endmacro()

# ######################################################################################################################
message(STATUS "Setting c-ares: ${TRANTOR_USE_C-ARES}")

if(TRANTOR_USE_C-ARES)
  # Set C-ARES_DIR, to let find_package could search from it
  if(WIN32)
    set(C-ARES_DIR
        c:/c-ares
        CACHE PATH "Let find_package use the directory to find" FORCE
    )
  else()
    set(C-ARES_DIR
        ${CMAKE_INSTALL_PREFIX}/c-ares
        CACHE PATH "Let find_package use the directory to find" FORCE
    )
  endif()

  find_package(c-ares)

  if(NOT c-ares_FOUND)
    if(FETCH_BUILD_MISSING_DEPS)
      message(STATUS "⚠️c-ares not found, Building it with FetchContent...")
      unset(c-ares_FOUND)

      build_install_cares()
      find_package(c-ares REQUIRED)

    else()
      message(FATAL_ERROR "⛔c-ares not found, please install it or set FETCH_BUILD_MISSING_DEPS to ON")
    endif()
  endif()

  set_cares_resolver()
else()
  set_normal_resolver()
endif()
