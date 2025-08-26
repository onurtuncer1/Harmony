# ------------------------------------------------------------------------------
# Copyright (c) 2025 Onur Tuncer, PhD, Istanbul Technical University
#
# SPDX-License-Identifier: BSD-3-Clause
# License-Filename: LICENSE
# ------------------------------------------------------------------------------

FIND_PROGRAM(CPPCHECK_PATH cppcheck)

IF(NOT CPPCHECK_PATH)
    MESSAGE(FATAL_ERROR "cppcheck not found! Aborting...")
ENDIF()

file(GLOB_RECURSE APP_SOURCES "${CMAKE_SOURCE_DIR}/src/*.cpp")

set(STATIC_ANALYSIS_SOURCE_DIR ${CMAKE_SOURCE_DIR}) 
set(STATIC_ANALYSIS_REPORT_DIR ${CMAKE_BINARY_DIR}/static_analysis)
file(MAKE_DIRECTORY ${STATIC_ANALYSIS_REPORT_DIR})
set(STATIC_ANALYSIS_XML_FILE ${STATIC_ANALYSIS_REPORT_DIR}/static_analysis_report.xml)

set(NAMESPACED_TARGET "static_analysis")
add_custom_target(${NAMESPACED_TARGET})

add_custom_command(TARGET ${NAMESPACED_TARGET}
  COMMAND cppcheck
   ARGS  --enable=all
         --force
         --suppress=unusedFunction
         --cppcheck-build-dir=${STATIC_ANALYSIS_REPORT_DIR}
         -I/${CMAKE_SOURCE_DIR}
         --suppressions-list=${CMAKE_BINARY_DIR}/../../tools/cppcheck/CppCheckSuppressions.txt
         --inconclusive
         --xml-version=2
         -v ${APP_SOURCES}
         --output-file=${STATIC_ANALYSIS_XML_FILE}
  COMMAND cppcheck-htmlreport
    ARGS    --title=${CMAKE_PROJECT_NAME}
            --file=${STATIC_ANALYSIS_XML_FILE}
            --report-dir=${STATIC_ANALYSIS_REPORT_DIR}
            --source-dir=${STATIC_ANALYSIS_SOURCE_DIR}
            --source-encoding=UTF-8  # Changed to UTF-8
    DEPENDS ${STATIC_ANALYSIS_XML_FILE}
  COMMENT "Running static analysis tool & generating html report"
  VERBATIM)

