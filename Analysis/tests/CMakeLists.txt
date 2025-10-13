# Analysis Test Suite
cmake_minimum_required(VERSION 3.5)

# Include directories for tests
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_BINARY_DIR}
    ${GTEST_INCLUDE_DIRS}
)

# Common test libraries
set(TEST_LIBRARIES 
    QwAnalysis
    ${GTEST_LIBRARIES}
    ${ROOT_LIBRARIES}
)

# Add compile flags
add_definitions(${GTEST_CFLAGS_OTHER})

#----------------------------------------------------------------------------
# Unit Tests
#
file(GLOB_RECURSE UNIT_TEST_SOURCES "unit/*.cpp")
if(UNIT_TEST_SOURCES)
    add_executable(analysis_unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(analysis_unit_tests ${TEST_LIBRARIES})
    target_include_directories(analysis_unit_tests PRIVATE ${GTEST_INCLUDE_DIRS})
    
    # Add test to CTest
    add_test(NAME AnalysisUnitTests COMMAND analysis_unit_tests)
    set_tests_properties(AnalysisUnitTests PROPERTIES
        TIMEOUT 300
        LABELS "unit;analysis"
    )
    
    # Add individual test discovery for better reporting
    gtest_discover_tests(analysis_unit_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES LABELS "unit;analysis"
    )
endif()

#----------------------------------------------------------------------------
# Integration Tests
#
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration/*.cpp")
if(INTEGRATION_TEST_SOURCES)
    add_executable(analysis_integration_tests ${INTEGRATION_TEST_SOURCES})
    target_link_libraries(analysis_integration_tests ${TEST_LIBRARIES})
    target_include_directories(analysis_integration_tests PRIVATE ${GTEST_INCLUDE_DIRS})
    
    # Add test to CTest
    add_test(NAME AnalysisIntegrationTests COMMAND analysis_integration_tests)
    set_tests_properties(AnalysisIntegrationTests PROPERTIES
        TIMEOUT 600
        LABELS "integration;analysis"
    )
    
    # Add individual test discovery
    gtest_discover_tests(analysis_integration_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES LABELS "integration;analysis"
    )
endif()

#----------------------------------------------------------------------------
# Benchmarks
#
if(ENABLE_BENCHMARKING AND BENCHMARK_FOUND)
    file(GLOB_RECURSE BENCHMARK_SOURCES "benchmarks/*.cpp")
    if(BENCHMARK_SOURCES)
        add_executable(analysis_benchmarks ${BENCHMARK_SOURCES})
        target_link_libraries(analysis_benchmarks 
            QwAnalysis
            ${BENCHMARK_LIBRARIES}
            ${ROOT_LIBRARIES}
        )
        target_include_directories(analysis_benchmarks PRIVATE ${BENCHMARK_INCLUDE_DIRS})
        
        # Add benchmark as a test (but don't run in regular testing)
        add_test(NAME AnalysisBenchmarks COMMAND analysis_benchmarks --benchmark_format=console)
        set_tests_properties(AnalysisBenchmarks PROPERTIES
            TIMEOUT 1800
            LABELS "benchmark;analysis"
        )
        
        # Create separate benchmark targets for different categories
        add_custom_target(analysis_benchmarks_quick
            COMMAND analysis_benchmarks --benchmark_filter="BM_.*Channel.*" --benchmark_format=console
            DEPENDS analysis_benchmarks
            COMMENT "Running quick Analysis framework benchmarks"
        )
        
        add_custom_target(analysis_benchmarks_full
            COMMAND analysis_benchmarks --benchmark_format=json --benchmark_out=analysis_benchmark_results.json
            DEPENDS analysis_benchmarks
            COMMENT "Running full Analysis framework benchmarks with JSON output"
        )
    endif()
endif()

#----------------------------------------------------------------------------
# Test Data and Utilities
#

# Copy test data files if they exist
file(GLOB TEST_DATA_FILES "data/*")
if(TEST_DATA_FILES)
    foreach(TEST_FILE ${TEST_DATA_FILES})
        get_filename_component(TEST_FILENAME ${TEST_FILE} NAME)
        configure_file(${TEST_FILE} ${CMAKE_CURRENT_BINARY_DIR}/data/${TEST_FILENAME} COPYONLY)
    endforeach()
endif()

# Create test output directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_output)

#----------------------------------------------------------------------------
# Test Coverage (if enabled)
#
if(ENABLE_COVERAGE)
    if(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        target_compile_options(analysis_unit_tests PRIVATE --coverage)
        target_link_libraries(analysis_unit_tests --coverage)
        
        if(TARGET analysis_integration_tests)
            target_compile_options(analysis_integration_tests PRIVATE --coverage)
            target_link_libraries(analysis_integration_tests --coverage)
        endif()
        
        # Add coverage target
        add_custom_target(analysis_coverage
            COMMAND lcov --directory . --capture --output-file analysis_coverage.info
            COMMAND lcov --remove analysis_coverage.info '/usr/*' --output-file analysis_coverage.info
            COMMAND lcov --list analysis_coverage.info
            COMMAND genhtml -o analysis_coverage analysis_coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating Analysis test coverage report"
        )
        
        # Analysis-specific coverage with proper filtering
        add_custom_target(analysis_coverage_report
            COMMAND lcov --directory . --zerocounters
            COMMAND ${CMAKE_CTEST_COMMAND} -L "analysis" --output-on-failure
            COMMAND lcov --directory . --capture --output-file analysis_detailed.info
            COMMAND lcov --extract analysis_detailed.info '*/Analysis/*' --output-file analysis_filtered.info
            COMMAND lcov --remove analysis_filtered.info '*/tests/*' --output-file analysis_filtered.info
            COMMAND genhtml analysis_filtered.info --output-directory analysis_coverage_html --title "Analysis Framework Coverage"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating detailed Analysis framework coverage report"
        )
        
    endif()
endif()

#----------------------------------------------------------------------------
# Custom Test Targets
#

# Quick unit test target
add_custom_target(analysis_test_quick
    COMMAND analysis_unit_tests --gtest_filter="*Basic*:*Construction*:*Simple*"
    DEPENDS analysis_unit_tests
    COMMENT "Running quick Analysis unit tests"
)

# Memory check target (if valgrind is available)
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
    add_custom_target(analysis_test_memcheck
        COMMAND ${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full --show-reachable=yes 
                --error-exitcode=1 $<TARGET_FILE:analysis_unit_tests>
        DEPENDS analysis_unit_tests
        COMMENT "Running Analysis unit tests with memory checking"
    )
endif()

# Performance regression test
if(TARGET analysis_benchmarks)
    add_custom_target(analysis_performance_regression
        COMMAND analysis_benchmarks --benchmark_filter="BM_.*Channel.*" 
                --benchmark_format=json --benchmark_out=current_performance.json
        COMMAND ${CMAKE_COMMAND} -E echo "Compare current_performance.json with baseline for regressions"
        DEPENDS analysis_benchmarks
        COMMENT "Running Analysis performance regression tests"
    )
endif()

#----------------------------------------------------------------------------
# Test Installation
#
if(BUILD_TESTING)
    install(TARGETS analysis_unit_tests
        RUNTIME DESTINATION bin/tests
        COMPONENT testing
    )
    
    if(TARGET analysis_integration_tests)
        install(TARGETS analysis_integration_tests
            RUNTIME DESTINATION bin/tests
            COMPONENT testing
        )
    endif()
    
    if(TARGET analysis_benchmarks)
        install(TARGETS analysis_benchmarks
            RUNTIME DESTINATION bin/benchmarks
            COMPONENT testing
        )
    endif()
endif()
