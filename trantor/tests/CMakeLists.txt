add_executable(ssl_server_test SSLServerTest.cc)
target_link_libraries(ssl_server_test PRIVATE trantor pthread)

add_executable(ssl_client_test SSLClientTest.cc)
target_link_libraries(ssl_client_test PRIVATE trantor pthread)

add_executable(serial_task_queue_test1 SerialTaskQueueTest1.cc)
target_link_libraries(serial_task_queue_test1 PRIVATE trantor pthread)

add_executable(serial_task_queue_test2 SerialTaskQueueTest2.cc)
target_link_libraries(serial_task_queue_test2 PRIVATE trantor pthread)

add_executable(timer_test TimerTest.cc)
target_link_libraries(timer_test PRIVATE trantor pthread)

add_executable(timer_test1 TimerTest1.cc)
target_link_libraries(timer_test1 PRIVATE trantor pthread)

add_executable(run_in_loop_test1 RunInLoopTest1.cc)
target_link_libraries(run_in_loop_test1 PRIVATE trantor pthread)

add_executable(run_in_loop_test2 RunInLoopTest2.cc)
target_link_libraries(run_in_loop_test2 PRIVATE trantor pthread)

add_executable(logger_test LoggerTest.cc)
target_link_libraries(logger_test PRIVATE trantor pthread)

add_executable(async_file_logger_test AsyncFileLoggerTest.cc)
target_link_libraries(async_file_logger_test PRIVATE trantor pthread)

add_executable(tcp_server_test TcpServerTest.cc)
target_link_libraries(tcp_server_test PRIVATE trantor pthread)

add_executable(concurrent_task_queue_test ConcurrentTaskQueueTest.cc)
target_link_libraries(concurrent_task_queue_test PRIVATE trantor pthread)

add_executable(tcp_client_test TcpClientTest.cc)
target_link_libraries(tcp_client_test PRIVATE trantor pthread)

add_executable(async_file_logger_test1 AsyncFileLoggerTest1.cc)
target_link_libraries(async_file_logger_test1 PRIVATE trantor pthread)

add_executable(sendfile_test SendfileTest.cc)
target_link_libraries(sendfile_test PRIVATE trantor pthread)

add_executable(timing_wheel_test TimingWheelTest.cc)
target_link_libraries(timing_wheel_test PRIVATE trantor pthread)

add_executable(kickoff_test KickoffTest.cc)
target_link_libraries(kickoff_test PRIVATE trantor pthread)

add_executable(dns_test DnsTest.cc)
target_link_libraries(dns_test PRIVATE trantor pthread)

