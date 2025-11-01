cmake_minimum_required(VERSION 4.0)
project(vector)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(vector main.cpp)
add_executable(vector_tests tests/vector_test.cpp)

target_link_libraries(vector_tests PRIVATE GTest::gtest_main)
enable_testing()
include(GoogleTest)
gtest_discover_tests(vector_tests)