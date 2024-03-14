# set normal resolver
macro(set_normal_resolver)
  list(APPEND TRANTOR_SOURCES trantor/net/inner/NormalResolver.cc)
  list(APPEND PRIVATE_HEADERS trantor/net/inner/NormalResolver.h)
endmacro()

# set cares resolver
macro(set_cares_resolver)
  if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CARES_STATICLIB)
  endif()

  target_link_libraries(${PROJECT_NAME} PRIVATE c-ares::cares)

  if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE resolv)
  elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Iphlpapi)
  endif()

  list(APPEND TRANTOR_SOURCES trantor/net/inner/AresResolver.cc)
  list(APPEND PRIVATE_HEADERS trantor/net/inner/AresResolver.h)
endmacro()

# ######################################################################################################################
message(STATUS "Setting c-ares: ${TRANTOR_USE_C-ARES}")

if(TRANTOR_USE_C-ARES)
  if(NOT BUILD_DEPENDENCIES)
    find_package(c-ares REQUIRED)
  endif()

  set_cares_resolver()
else()
  set_normal_resolver()
endif()
