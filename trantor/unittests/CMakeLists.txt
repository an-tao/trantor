add_executable(msgbuffer_unittest MsgBufferUnittest.cc)
add_executable(inetaddress_unittest InetAddressUnittest.cc)
add_executable(date_unittest DateUnittest.cc)
add_executable(split_string_unittest splitStringUnittest.cc)
add_executable(string_encoding_unittest stringEncodingUnittest.cc)
add_executable(hash_unittest HashUnittest.cc)

set(UNITTEST_TARGETS
    split_string_unittest
    string_encoding_unittest
    date_unittest
    hash_unittest
    inetaddress_unittest
    msgbuffer_unittest
)

if(NOT
   TRANTOR_TLS_PROVIDER
   STREQUAL
   "none"
)
  add_executable(ssl_name_verify_unittest sslNameVerifyUnittest.cc)
  list(APPEND UNITTEST_TARGETS ssl_name_verify_unittest)
endif()

set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_EXTENSIONS OFF)

include(GoogleTest)
foreach(T ${UNITTEST_TARGETS})
  message(STATUS "Add Testing ${T}")
  target_link_libraries(${T} PRIVATE ${PROJECT_NAME} GTest::gtest_main)

  gtest_discover_tests(${T})
endforeach()
