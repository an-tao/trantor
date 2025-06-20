# build and install spdlog
macro(add_spdlog)
  if(NOT spdlog_DOWNLOAD_URL)
    get_github_latest_release_url("gabime" "spdlog")
  endif()

  FetchContent_Declare(spdlog URL ${spdlog_DOWNLOAD_URL})
  FetchContent_MakeAvailable(spdlog)
endmacro()

macro(set_spdlog)
  target_include_directories(${PROJECT_NAME} PUBLIC ${spdlog_INCLUDE_DIR})

  target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
  target_compile_definitions(${PROJECT_NAME} PUBLIC TRANTOR_SPDLOG_SUPPORT)
endmacro()

# ######################################################################################################################
message(STATUS "Setting for spdlog: ${TRANTOR_USE_SPDLOG}")

if(TRANTOR_USE_SPDLOG)
  find_package(spdlog)

  if(NOT spdlog_FOUND)
    if(FETCH_BUILD_MISSING_DEPS)
      message(STATUS "⚠️spdlog not found, Building it with FetchContent...")
      unset(spdlog_FOUND)

      add_spdlog()
    else()
      message(FATAL_ERROR "⛔spdlog not found, please install it or set FETCH_BUILD_MISSING_DEPS to ON")
    endif()
  endif()

  set_spdlog()
endif()
