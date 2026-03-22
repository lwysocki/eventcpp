Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.0)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.9.5
  SYSTEM)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

set(target_name eventcpp_test)

add_executable(${target_name} sanity_test.cpp
                              free_functions_test.cpp
                              member_functions_test.cpp
                              polymorphism_test.cpp
                              signature_test.cpp
                              multi_subscriber_test.cpp)
target_link_libraries(${target_name} PRIVATE eventcpp Catch2::Catch2WithMain)

set(target_benchmark eventcpp_benchmark)

add_executable(${target_benchmark} benchmark_test.cpp)
target_link_libraries(${target_benchmark} PRIVATE eventcpp benchmark::benchmark_main)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${target_name})
