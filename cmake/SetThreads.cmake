message(STATUS "Setting Threads")

find_package(Threads)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
