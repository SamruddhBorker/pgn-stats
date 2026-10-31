cmake_minimum_required(VERSION 3.16)
project(pgn-stats VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(PGNSTATS_BUILD_TESTS "Build the unit tests" ON)

if(MSVC)
  set(PGNSTATS_WARNINGS /W4 /permissive- /utf-8)
else()
  set(PGNSTATS_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_library(pgnstats STATIC
  src/tokenizer.cpp
  src/parser.cpp
  src/opening_trie.cpp
  src/stats.cpp
  src/report.cpp
)
target_include_directories(pgnstats PUBLIC include)
target_compile_options(pgnstats PRIVATE ${PGNSTATS_WARNINGS})

add_executable(pgn-stats src/main.cpp)
target_link_libraries(pgn-stats PRIVATE pgnstats)
target_compile_options(pgn-stats PRIVATE ${PGNSTATS_WARNINGS})

if(PGNSTATS_BUILD_TESTS)
  enable_testing()

  add_executable(pgnstats_tests
    tests/test_main.cpp
    tests/test_tokenizer.cpp
    tests/test_parser.cpp
    tests/test_opening_trie.cpp
    tests/test_stats.cpp
  )
  target_link_libraries(pgnstats_tests PRIVATE pgnstats)
  target_include_directories(pgnstats_tests PRIVATE third_party)
  target_compile_definitions(pgnstats_tests PRIVATE
    PGNSTATS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

  add_test(NAME unit_tests COMMAND pgnstats_tests)

  # End-to-end checks of the CLI itself.
  add_test(NAME cli_summary COMMAND pgn-stats ${CMAKE_CURRENT_SOURCE_DIR}/data/classics.pgn --top 3)
  set_tests_properties(cli_summary PROPERTIES PASS_REGULAR_EXPRESSION "Games: 12")

  add_test(NAME cli_player COMMAND pgn-stats ${CMAKE_CURRENT_SOURCE_DIR}/data/classics.pgn --player morphy)
  set_tests_properties(cli_player PROPERTIES PASS_REGULAR_EXPRESSION "Record: \\+3 =0 -0")

  add_test(NAME cli_missing_file COMMAND pgn-stats does-not-exist.pgn)
  set_tests_properties(cli_missing_file PROPERTIES WILL_FAIL TRUE)

  add_test(NAME cli_bad_usage COMMAND pgn-stats --top)
  set_tests_properties(cli_bad_usage PROPERTIES WILL_FAIL TRUE)
endif()
