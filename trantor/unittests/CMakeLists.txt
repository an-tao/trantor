find_package(GTest REQUIRED)
add_executable(msgbuffer_unittest MsgBufferUnittest.cc)
add_executable(inetaddress_unittest InetAddressUnittest.cc)
add_executable(date_unittest DateUnittest.cc)
add_executable(split_string_unittest splitStringUnittest.cc)
add_executable(string_encoding_unittest stringEncodingUnittest.cc)
add_executable(hash_unittest HashUnittest.cc)
add_executable(tcp_client_retry_unittest TcpClientRetryUnittest.cc)
add_executable(tcp_connection_unittest TcpConnectionUnittest.cc)
set(UNITTEST_TARGETS
    msgbuffer_unittest
    inetaddress_unittest
    date_unittest
    split_string_unittest
    string_encoding_unittest
    hash_unittest
    tcp_client_retry_unittest
    tcp_connection_unittest
)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${UNITTEST_TARGETS} PROPERTY CXX_EXTENSIONS OFF)

if(WIN32)
  target_link_libraries(tcp_connection_unittest PRIVATE ws2_32)
endif()

include(GoogleTest)
foreach(T ${UNITTEST_TARGETS})
  target_link_libraries(${T} PRIVATE trantor GTest::GTest)
  # Fix for Windows when using shared libraries: trantor.dll not in the path of the test executables
  if(WIN32
     AND BUILD_SHARED_LIBS
     AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.29
  )
    # Fix for CMake >= 3.29: add the path to the trantor DLL to the test executables's PATH, so that they find it at
    # runtime.
    get_property(
      test_launcher
      TARGET ${T}
      PROPERTY TEST_LAUNCHER
    )
    set(trantor_test_launcher
        "${CMAKE_COMMAND}"
        -E
        env
        --modify
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:trantor>"
        --
    )
    # Keep any user-supplied launcher behind the per-process PATH wrapper.
    if(NOT
       "${test_launcher}"
       STREQUAL
       ""
    )
      list(APPEND trantor_test_launcher "${test_launcher}")
    endif()
    set_property(TARGET ${T} PROPERTY TEST_LAUNCHER "${trantor_test_launcher}")
  endif()
  gtest_discover_tests(${T})
endforeach()

if(WIN32 AND BUILD_SHARED_LIBS)
  # This test exercises private Connector code, which is not exported by the shared library on Windows.
  target_sources(tcp_client_retry_unittest PRIVATE ../net/inner/Connector.cc ../net/inner/Socket.cc)
  target_include_directories(tcp_client_retry_unittest PRIVATE ../net ../net/inner)
  # The test itself calls Winsock APIs directly.
  target_link_libraries(tcp_client_retry_unittest PRIVATE ws2_32)
endif()
