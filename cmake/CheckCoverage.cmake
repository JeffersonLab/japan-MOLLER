# Coverage threshold checking script
# This script checks if coverage meets minimum thresholds

set(COVERAGE_INFO_FILE "coverage_filtered.info")
set(MIN_LINE_COVERAGE 70)    # Minimum line coverage percentage
set(MIN_FUNCTION_COVERAGE 60) # Minimum function coverage percentage
set(MIN_BRANCH_COVERAGE 50)   # Minimum branch coverage percentage

# Check if coverage info file exists
if(NOT EXISTS "${COVERAGE_INFO_FILE}")
    message(FATAL_ERROR "Coverage info file not found: ${COVERAGE_INFO_FILE}")
endif()

# Extract coverage statistics using lcov
find_program(LCOV_EXECUTABLE lcov)
if(NOT LCOV_EXECUTABLE)
    message(FATAL_ERROR "lcov not found")
endif()

# Get coverage summary
execute_process(
    COMMAND ${LCOV_EXECUTABLE} --summary ${COVERAGE_INFO_FILE}
    OUTPUT_VARIABLE COVERAGE_SUMMARY
    ERROR_VARIABLE COVERAGE_ERROR
    RESULT_VARIABLE COVERAGE_RESULT
)

if(NOT COVERAGE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to get coverage summary: ${COVERAGE_ERROR}")
endif()

# Parse line coverage
string(REGEX MATCH "lines\\.*: ([0-9.]+)%" LINE_MATCH "${COVERAGE_SUMMARY}")
if(LINE_MATCH)
    set(LINE_COVERAGE ${CMAKE_MATCH_1})
else()
    message(FATAL_ERROR "Could not parse line coverage from summary")
endif()

# Parse function coverage
string(REGEX MATCH "functions\\.*: ([0-9.]+)%" FUNCTION_MATCH "${COVERAGE_SUMMARY}")
if(FUNCTION_MATCH)
    set(FUNCTION_COVERAGE ${CMAKE_MATCH_1})
else()
    message(WARNING "Could not parse function coverage from summary")
    set(FUNCTION_COVERAGE 0)
endif()

# Parse branch coverage
string(REGEX MATCH "branches\\.*: ([0-9.]+)%" BRANCH_MATCH "${COVERAGE_SUMMARY}")
if(BRANCH_MATCH)
    set(BRANCH_COVERAGE ${CMAKE_MATCH_1})
else()
    message(WARNING "Could not parse branch coverage from summary")
    set(BRANCH_COVERAGE 0)
endif()

# Display results
message(STATUS "Coverage Results:")
message(STATUS "  Line Coverage:     ${LINE_COVERAGE}% (threshold: ${MIN_LINE_COVERAGE}%)")
message(STATUS "  Function Coverage: ${FUNCTION_COVERAGE}% (threshold: ${MIN_FUNCTION_COVERAGE}%)")
message(STATUS "  Branch Coverage:   ${BRANCH_COVERAGE}% (threshold: ${MIN_BRANCH_COVERAGE}%)")

# Check thresholds
set(COVERAGE_PASS TRUE)

if(LINE_COVERAGE LESS MIN_LINE_COVERAGE)
    message(SEND_ERROR "Line coverage ${LINE_COVERAGE}% is below threshold ${MIN_LINE_COVERAGE}%")
    set(COVERAGE_PASS FALSE)
endif()

if(FUNCTION_COVERAGE LESS MIN_FUNCTION_COVERAGE)
    message(SEND_ERROR "Function coverage ${FUNCTION_COVERAGE}% is below threshold ${MIN_FUNCTION_COVERAGE}%")
    set(COVERAGE_PASS FALSE)
endif()

if(BRANCH_COVERAGE LESS MIN_BRANCH_COVERAGE)
    message(SEND_ERROR "Branch coverage ${BRANCH_COVERAGE}% is below threshold ${MIN_BRANCH_COVERAGE}%")
    set(COVERAGE_PASS FALSE)
endif()

if(COVERAGE_PASS)
    message(STATUS "All coverage thresholds met!")
    # Write success marker file
    file(WRITE "coverage_passed.txt" "Coverage thresholds met\nLine: ${LINE_COVERAGE}%\nFunction: ${FUNCTION_COVERAGE}%\nBranch: ${BRANCH_COVERAGE}%\n")
else()
    message(FATAL_ERROR "Coverage thresholds not met!")
endif()
