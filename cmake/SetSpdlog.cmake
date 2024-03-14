message(STATUS "Setting for spdlog: ${TRANTOR_USE_SPDLOG}")

if(TRANTOR_USE_SPDLOG)
  target_compile_definitions(${PROJECT_NAME} PUBLIC TRANTOR_SPDLOG_SUPPORT)

  if(BUILD_DEPENDENCIES)
    target_include_directories(
      ${PROJECT_NAME}
      PRIVATE
      PUBLIC $<BUILD_INTERFACE:${spdlog_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

  else()
    find_package(spdlog REQUIRED)

    target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)
    target_compile_definitions(${PROJECT_NAME} PUBLIC TRANTOR_SPDLOG_SUPPORT SPDLOG_FMT_EXTERNAL SPDLOG_FMT_EXTERNAL_HO)
  endif()
endif()
