link_libraries(trantor pthread)

if(OpenSSL_FOUND)
    add_executable(ssl_server_test SSLServerTest.cc)
    add_executable(ssl_client_test SSLClientTest.cc)
endif()

add_executable(serial_task_queue_test1 SerialTaskQueueTest1.cc)

add_executable(serial_task_queue_test2 SerialTaskQueueTest2.cc)

add_executable(timer_test TimerTest.cc)

add_executable(timer_test1 TimerTest1.cc)

add_executable(run_in_loop_test1 RunInLoopTest1.cc)

add_executable(run_in_loop_test2 RunInLoopTest2.cc)

add_executable(logger_test LoggerTest.cc)

add_executable(async_file_logger_test AsyncFileLoggerTest.cc)

add_executable(tcp_server_test TcpServerTest.cc)

add_executable(concurrent_task_queue_test ConcurrentTaskQueueTest.cc)

add_executable(tcp_client_test TcpClientTest.cc)

add_executable(async_file_logger_test1 AsyncFileLoggerTest1.cc)

add_executable(sendfile_test SendfileTest.cc)

add_executable(timing_wheel_test TimingWheelTest.cc)

add_executable(kickoff_test KickoffTest.cc)


