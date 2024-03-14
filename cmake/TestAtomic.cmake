message(STATUS "Testing Atomic compilation")

file(WRITE ${CMAKE_BINARY_DIR}/test_atomic.cpp "#include <atomic>\n"
                                               "int main() { std::atomic<int64_t> i(0); i++; return 0; }\n"
)
try_compile(ATOMIC_WITHOUT_LINKING ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/test_atomic.cpp)

if(NOT ATOMIC_WITHOUT_LINKING)
  target_link_libraries(${PROJECT_NAME} PUBLIC atomic)
endif()

file(REMOVE ${CMAKE_BINARY_DIR}/test_atomic.cpp)
