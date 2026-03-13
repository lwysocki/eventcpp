Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.0)

FetchContent_MakeAvailable(Catch2)

set(target_name eventcpp_test)

add_executable(${target_name} event_basic.cpp
                              return_types.cpp
                              notify_with_params.cpp
                              notify_multiple.cpp)
target_link_libraries(${target_name} PRIVATE eventcpp Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(${target_name})
