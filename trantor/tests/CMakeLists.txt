set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(serial_task_queue_test SerialTaskQueueTest.cc)
target_link_libraries(serial_task_queue_test trantor)

add_executable(timer_test TimerTest.cc)
target_link_libraries(timer_test trantor)