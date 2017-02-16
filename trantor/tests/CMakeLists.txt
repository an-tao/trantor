link_libraries(trantor pthread)

add_executable(serial_task_queue_test SerialTaskQueueTest.cc)

add_executable(timer_test TimerTest.cc)

add_executable(timer_test1 TimerTest1.cc)

add_executable(run_in_loop_test RunInloopTest.cc)

add_executable(logger_test LoggerTest.cc)

add_executable(async_file_logger_test AsyncFileLoggerTest.cc)

add_executable(tcp_server_test TcpServerTest.cc)

add_executable(concurrent_task_queue_test ConcurrentTaskQueueTest.cc)
