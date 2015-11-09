project(libmacro)

cmake_minimum_required(VERSION 3.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules)

find_package(GOOGLE_TEST REQUIRED)
find_package(GOOGLE_BENCHMARK REQUIRED)

add_library(macro libmacro.cc)
target_compile_options(macro PUBLIC -std=c++11)

add_executable(libmacro-test libmacro-test-obj-like.cc libmacro-test-func-like.cc)
target_compile_options(libmacro-test PUBLIC -std=c++11)
target_include_directories(libmacro-test PUBLIC  ${GOOGLE_TEST_DIR}/include)
target_link_libraries(libmacro-test
  macro
  -L${GOOGLE_TEST_DIR}/lib gtest gtest_main
  pthread)

add_executable(libmacro-benchmark libmacro-benchmark.cc)
target_compile_options(libmacro-benchmark PUBLIC -std=c++11)
target_include_directories(libmacro-benchmark PUBLIC ${GOOGLE_BENCHMARK_DIR}/include)
target_link_libraries(libmacro-benchmark
  macro
  -L${GOOGLE_BENCHMARK_DIR}/lib benchmark
  pthread)
