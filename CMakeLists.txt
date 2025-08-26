# ------------------------------------------------------------------------------
# Project: Harmony Geometry Serialization De-serialization Library
# Copyright (c) 2025, Onur Tuncer, PhD, Istanbul Technical University
#
# SPDX-License-Identifier: BSD-3-Clause
# License-Filename: LICENSE
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.22)

project(Harmony
  VERSION 1.0.0
  LANGUAGES CXX
  DESCRIPTION "Geometry serialization and de-serialization"
)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_library(${PROJECT_NAME})
add_library(Harmony ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${PROJECT_NAME}>
)

target_sources(${PROJECT_NAME} PRIVATE
 # src/Logger.cpp

)


if (BUILD_DOCS)
  add_subdirectory(doc)
endif()

if (ENABLE_STATIC_ANALYSIS)
  include(cppcheck)
endif()

if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
