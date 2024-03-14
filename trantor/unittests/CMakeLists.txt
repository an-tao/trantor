add_executable(hash_unittest HashUnittest.cc)
add_executable(date_unittest DateUnittest.cc)
add_executable(inetaddress_unittest InetAddressUnittest.cc)
add_executable(msgbuffer_unittest MsgBufferUnittest.cc)
add_executable(split_string_unittest splitStringUnittest.cc)
add_executable(string_encoding_unittest stringEncodingUnittest.cc)

set(UNITTEST_TARGETS
    hash_unittest
    date_unittest
    inetaddress_unittest
    msgbuffer_unittest
    split_string_unittest
    string_encoding_unittest
)

if(TRANTOR_USE_TLS)
  add_executable(ssl_name_verify_unittest sslNameVerifyUnittest.cc)
  list(APPEND UNITTEST_TARGETS ssl_name_verify_unittest)
endif()

set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_EXTENSIONS OFF)

if(NOT BUILD_DEPENDENCIES)
  find_package(GTest REQUIRED)
endif()

include(GoogleTest)

foreach(T ${UNITTEST_TARGETS})
  if(BUILD_DEPENDENCIES)
    target_include_directories(${T} PRIVATE ${gtest_SOURCE_DIR}/include)
  endif()

  target_link_libraries(${T} PRIVATE trantor GTest::gtest_main)
  set_standard_build_output(${T})

  gtest_discover_tests(${T})
endforeach()
