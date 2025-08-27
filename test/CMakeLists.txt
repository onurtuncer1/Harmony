# ------------------------------------------------------------------------------
# Project: Harmony Geometry Serialization Deserialization Library
# Copyright (c) 2025, Onur Tuncer, PhD, Istanbul Technical University
#
# SPDX-License-Identifier: BSD-3-Clause
# License-Filename: LICENSE
# ------------------------------------------------------------------------------

find_package(Catch2 REQUIRED)

add_executable(${PROJECT_NAME}Tests
  test_AsciiSTL.cpp
  test_BinarySTL.cpp
)

target_link_libraries(${PROJECT_NAME}Tests PRIVATE
  ${PROJECT_NAME}
  Catch2::Catch2WithMain
)

enable_testing()
add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)

