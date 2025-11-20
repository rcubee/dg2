cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Note: When cross-compiling for platforms where syscalls are not provided, CMake will fail to build a simple test program
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(dg2)

add_library(dg2 STATIC)

target_compile_options(dg2 PRIVATE -Wall -Werror -Werror=switch-enum)

if({CMAKE_BUILD_TYPE} STREQUAL "Release")
target_compile_definitions(dg2 PRIVATE DG2_ENABLE_ASSERT=OFF)
target_compile_options(dg2 PRIVATE -Os)
else()
target_compile_definitions(dg2 PRIVATE DG2_ENABLE_ASSERT=ON)
target_compile_options(dg2 PRIVATE -Og)
endif()

target_sources(dg2 PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/src/dg2.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/dg2_circ_buff.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/dg2_crc.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/dg2_disp.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/dg2_pkt.c"
)

target_include_directories(dg2 PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

if(DG2_ENABLE_TESTS)
    message("Enabled tests")
    add_subdirectory(tests)
endif()
