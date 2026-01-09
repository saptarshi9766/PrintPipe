cmake_minimum_required(VERSION 3.20)

project(
  printpipe
  VERSION 0.1.0
  DESCRIPTION "OS-agnostic virtual printer pipeline (PrintPipe)"
  LANGUAGES CXX
)

# -----------------------------
# Global C++ configuration
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# -----------------------------
# Sanitizers (optional, Debug only)
# -----------------------------
option(PRINTPIPE_ENABLE_SANITIZERS "Enable Address/Undefined sanitizers" ON)

function(printpipe_enable_sanitizers target_name)
  if (PRINTPIPE_ENABLE_SANITIZERS AND
      CMAKE_BUILD_TYPE STREQUAL "Debug" AND
      CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined)
    target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

# -----------------------------
# Core library
# -----------------------------
add_library(printpipe
  src/job.cpp
  src/scheduler.cpp
  src/spooler.cpp
  src/file_backend.cpp
)

target_include_directories(printpipe
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

printpipe_enable_sanitizers(printpipe)

# -----------------------------
# HTTP Server dependencies
# -----------------------------
include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3
)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(httplib json)

# HTTP Server library
add_library(printpipe_server
  src/print_server.cpp
)

target_link_libraries(printpipe_server
  PUBLIC
    printpipe
    httplib::httplib
    nlohmann_json::nlohmann_json
)

target_include_directories(printpipe_server
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

printpipe_enable_sanitizers(printpipe_server)

# -----------------------------
# Testing (Catch2)
# -----------------------------

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)

FetchContent_MakeAvailable(Catch2)

enable_testing()

add_executable(printpipe_tests
  tests/test_job.cpp
)

target_link_libraries(printpipe_tests
  PRIVATE
    printpipe
    Catch2::Catch2WithMain
)

printpipe_enable_sanitizers(printpipe_tests)

add_test(
  NAME printpipe_tests
  COMMAND printpipe_tests
)
# -----------------------------
# Examples
# -----------------------------
add_executable(printpipe_demo
  examples/demo.cpp
)

target_link_libraries(printpipe_demo
  PRIVATE
    printpipe
)

printpipe_enable_sanitizers(printpipe_demo)

# -----------------------------
# HTTP Server Application
# -----------------------------
add_executable(printpipe_http_server
  src/http_server_main.cpp
)

target_link_libraries(printpipe_http_server
  PRIVATE
    printpipe_server
)

printpipe_enable_sanitizers(printpipe_http_server)

printpipe_enable_sanitizers(printpipe_demo)
