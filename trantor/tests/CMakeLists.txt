set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -o2 -g")

add_executable(serial_task_queue_test SerialTaskQueueTest.cc)
target_link_libraries(serial_task_queue_test trantor)

add_executable(timer_test TimerTest.cc)
target_link_libraries(timer_test trantor)

add_executable(timer_test1 TimerTest1.cc)
target_link_libraries(timer_test1 trantor)

add_executable(run_in_loop_test RunInloopTest.cc)
target_link_libraries(run_in_loop_test trantor)

add_executable(logger_test LoggerTest.cc)
target_link_libraries(logger_test trantor)

add_executable(async_file_logger_test AsyncFileLoggerTest.cc)
target_link_libraries(async_file_logger_test trantor)