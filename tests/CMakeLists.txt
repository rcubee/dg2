cmake_minimum_required(VERSION 3.14)
project(dg2_tests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/6910c9d9165801d8827d628cb72eb7ea9dd538c5.zip # googletest v1.16.x
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    tests
    tests.cpp
)

target_include_directories(tests PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(
  tests
  GTest::gtest_main
  dg2
)

include(GoogleTest)
gtest_discover_tests(tests)
