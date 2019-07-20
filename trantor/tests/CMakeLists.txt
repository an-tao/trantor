LINK_LIBRARIES(trantor pthread)

ADD_EXECUTABLE(ssl_server_test SSLServerTest.cc)

ADD_EXECUTABLE(ssl_client_test SSLClientTest.cc)

ADD_EXECUTABLE(serial_task_queue_test1 SerialTaskQueueTest1.cc)

ADD_EXECUTABLE(serial_task_queue_test2 SerialTaskQueueTest2.cc)

ADD_EXECUTABLE(timer_test TimerTest.cc)

ADD_EXECUTABLE(timer_test1 TimerTest1.cc)

ADD_EXECUTABLE(run_in_loop_test1 RunInLoopTest1.cc)

ADD_EXECUTABLE(run_in_loop_test2 RunInLoopTest2.cc)

ADD_EXECUTABLE(logger_test LoggerTest.cc)

ADD_EXECUTABLE(async_file_logger_test AsyncFileLoggerTest.cc)

ADD_EXECUTABLE(tcp_server_test TcpServerTest.cc)

ADD_EXECUTABLE(concurrent_task_queue_test ConcurrentTaskQueueTest.cc)

ADD_EXECUTABLE(tcp_client_test TcpClientTest.cc)

ADD_EXECUTABLE(async_file_logger_test1 AsyncFileLoggerTest1.cc)

ADD_EXECUTABLE(sendfile_test SendfileTest.cc)

ADD_EXECUTABLE(timing_wheel_test TimingWheelTest.cc)

ADD_EXECUTABLE(kickoff_test KickoffTest.cc)

SET(targets_list    ssl_server_test
                    ssl_client_test
                    serial_task_queue_test1
                    serial_task_queue_test2
                    timer_test
                    timer_test1
                    run_in_loop_test1
                    run_in_loop_test2
                    logger_test
                    async_file_logger_test
                    tcp_server_test
                    concurrent_task_queue_test
                    tcp_client_test
                    async_file_logger_test1
                    sendfile_test
                    timing_wheel_test
                    kickoff_test)

SET_PROPERTY(TARGET ${targets_list} PROPERTY CXX_STANDARD 14)
SET_PROPERTY(TARGET ${targets_list} PROPERTY CXX_STANDARD_REQUIRED ON)
SET_PROPERTY(TARGET ${targets_list} PROPERTY CXX_EXTENSIONS OFF)

