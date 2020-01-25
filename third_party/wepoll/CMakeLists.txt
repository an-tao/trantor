project (wepoll C)

cmake_minimum_required (VERSION 3.5)

option (BUILD_TESTS "tests?" OFF)

set (headers Wepoll.h)
set (sources Wepoll.c)

add_library (wepoll STATIC ${sources})

set_target_properties(wepoll PROPERTIES EXPORT_NAME MMAN)

target_link_libraries(wepoll PRIVATE ws2_32)

install (TARGETS wepoll
					  EXPORT wepollTargets
					  RUNTIME DESTINATION bin
                      LIBRARY DESTINATION lib${LIB_SUFFIX}
                      ARCHIVE DESTINATION lib${LIB_SUFFIX})

install(EXPORT wepollTargets
        DESTINATION "${INSTALL_TRANTOR_CMAKE_DIR}"
        COMPONENT dev)

install (FILES ${headers} DESTINATION include/sys)



