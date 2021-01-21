# CMake build script for ZeroMQ tests
cmake_minimum_required(VERSION "2.8.1")

# On Windows: solution file will be called tests.sln
PROJECT(tests)

set(tests
        test_server
)

# add library and include dirs for all targets
link_libraries(uzmq ${OPTIONAL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
include_directories("${CMAKE_SOURCE_DIR}/../include" "${CMAKE_BINARY_DIR}")

foreach(test ${tests})
  add_executable(${test} ${test}.cpp)
  # per-test directories not generated on OS X / Darwin
  link_directories(${test} PRIVATE "${CMAKE_SOURCE_DIR}/../lib")
endforeach()
