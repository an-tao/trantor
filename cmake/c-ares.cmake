FetchContent_Declare(
  c-ares
  GIT_REPOSITORY https://github.com/c-ares/c-ares.git
  GIT_TAG v1.34.6 OVERRIDE_FIND_PACKAGE
)

set(CARES_STATIC
    ON
    CACHE BOOL "" FORCE
)
set(CARES_STATIC_PIC
    ON
    CACHE BOOL "" FORCE
)
set(CARES_SHARED
    OFF
    CACHE BOOL "" FORCE
)

set(c-ares_INCLUDE_DIRS "${FETCHCONTENT_BASE_DIR}/c-ares-build/" "${FETCHCONTENT_BASE_DIR}/c-ares-src/include/")
set(c-ares_LIBRARIES "${FETCHCONTENT_BASE_DIR}/c-ares-build/src/lib/")

mark_as_advanced(c-ares_INCLUDE_DIRS c-ares_LIBRARIES)

FetchContent_MakeAvailable(c-ares)

add_library(c-ares::c-ares ALIAS c-ares)
