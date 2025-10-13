# Parity Test Suite  
cmake_minimum_required(VERSION 3.5)

# Include directories for tests
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Analysis/include
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
    add_executable(parity_unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(parity_unit_tests ${TEST_LIBRARIES})
    target_include_directories(parity_unit_tests PRIVATE ${GTEST_INCLUDE_DIRS})
    
    # Add test to CTest
    add_test(NAME ParityUnitTests COMMAND parity_unit_tests)
    set_tests_properties(ParityUnitTests PROPERTIES
        TIMEOUT 300
        LABELS "unit;parity"
    )
    
    # Add individual test discovery for better reporting
    gtest_discover_tests(parity_unit_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES LABELS "unit;parity"
    )
endif()

#----------------------------------------------------------------------------
# Integration Tests
#
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration/*.cpp")
if(INTEGRATION_TEST_SOURCES)
    add_executable(parity_integration_tests ${INTEGRATION_TEST_SOURCES})
    target_link_libraries(parity_integration_tests ${TEST_LIBRARIES})
    target_include_directories(parity_integration_tests PRIVATE ${GTEST_INCLUDE_DIRS})
    
    # Add test to CTest
    add_test(NAME ParityIntegrationTests COMMAND parity_integration_tests)
    set_tests_properties(ParityIntegrationTests PROPERTIES
        TIMEOUT 600
        LABELS "integration;parity"
    )
    
    # Add individual test discovery
    gtest_discover_tests(parity_integration_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES LABELS "integration;parity"
    )
endif()

#----------------------------------------------------------------------------
# Benchmarks
#
if(ENABLE_BENCHMARKING AND BENCHMARK_FOUND)
    file(GLOB_RECURSE BENCHMARK_SOURCES "benchmarks/*.cpp")
    if(BENCHMARK_SOURCES)
        add_executable(parity_benchmarks ${BENCHMARK_SOURCES})
        target_link_libraries(parity_benchmarks 
            QwAnalysis
            ${BENCHMARK_LIBRARIES}
            ${ROOT_LIBRARIES}
        )
        target_include_directories(parity_benchmarks PRIVATE ${BENCHMARK_INCLUDE_DIRS})
        
        # Add benchmark as a test (but don't run in regular testing)
        add_test(NAME ParityBenchmarks COMMAND parity_benchmarks --benchmark_format=console)
        set_tests_properties(ParityBenchmarks PROPERTIES
            TIMEOUT 1800
            LABELS "benchmark;parity"
        )
        
        # Create separate benchmark targets for different categories
        add_custom_target(parity_benchmarks_helicity
            COMMAND parity_benchmarks --benchmark_filter="BM_Helicity.*" --benchmark_format=console
            DEPENDS parity_benchmarks
            COMMENT "Running Helicity processing benchmarks"
        )
        
        add_custom_target(parity_benchmarks_blinding
            COMMAND parity_benchmarks --benchmark_filter="BM_.*Blind.*" --benchmark_format=console
            DEPENDS parity_benchmarks
            COMMENT "Running Blinding operation benchmarks"
        )
        
        add_custom_target(parity_benchmarks_detectors
            COMMAND parity_benchmarks --benchmark_filter="BM_BPM.*|BM_BCM.*" --benchmark_format=console
            DEPENDS parity_benchmarks
            COMMENT "Running Detector processing benchmarks"
        )
        
        add_custom_target(parity_benchmarks_full
            COMMAND parity_benchmarks --benchmark_format=json --benchmark_out=parity_benchmark_results.json
            DEPENDS parity_benchmarks
            COMMENT "Running full Parity analysis benchmarks with JSON output"
        )
    endif()
endif()

#----------------------------------------------------------------------------
# Test Data and Utilities
#

# Copy test configuration files if they exist
file(GLOB TEST_CONFIG_FILES "data/*.conf" "data/*.map")
if(TEST_CONFIG_FILES)
    foreach(CONFIG_FILE ${TEST_CONFIG_FILES})
        get_filename_component(CONFIG_FILENAME ${CONFIG_FILE} NAME)
        configure_file(${CONFIG_FILE} ${CMAKE_CURRENT_BINARY_DIR}/data/${CONFIG_FILENAME} COPYONLY)
    endforeach()
endif()

# Create test output directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_output)

#----------------------------------------------------------------------------
# Test Coverage (if enabled)
#
if(ENABLE_COVERAGE)
    if(CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        target_compile_options(parity_unit_tests PRIVATE --coverage)
        target_link_libraries(parity_unit_tests --coverage)
        
        if(TARGET parity_integration_tests)
            target_compile_options(parity_integration_tests PRIVATE --coverage)
            target_link_libraries(parity_integration_tests --coverage)
        endif()
        
        # Add coverage target
        add_custom_target(parity_coverage
            COMMAND lcov --directory . --capture --output-file parity_coverage.info
            COMMAND lcov --remove parity_coverage.info '/usr/*' --output-file parity_coverage.info
            COMMAND lcov --list parity_coverage.info
            COMMAND genhtml -o parity_coverage parity_coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating Parity test coverage report"
        )
        
        # Parity-specific coverage with proper filtering
        add_custom_target(parity_coverage_report
            COMMAND lcov --directory . --zerocounters
            COMMAND ${CMAKE_CTEST_COMMAND} -L "parity" --output-on-failure
            COMMAND lcov --directory . --capture --output-file parity_detailed.info
            COMMAND lcov --extract parity_detailed.info '*/Parity/*' --output-file parity_filtered.info
            COMMAND lcov --remove parity_filtered.info '*/tests/*' --output-file parity_filtered.info
            COMMAND genhtml parity_filtered.info --output-directory parity_coverage_html --title "Parity Framework Coverage"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating detailed Parity framework coverage report"
        )
        
    endif()
endif()

#----------------------------------------------------------------------------
# Custom Test Targets
#

# Quick unit test target
add_custom_target(parity_test_quick
    COMMAND parity_unit_tests --gtest_filter="*Basic*:*Construction*:*Simple*"
    DEPENDS parity_unit_tests
    COMMENT "Running quick Parity unit tests"
)

# Helicity-specific tests
add_custom_target(parity_test_helicity
    COMMAND parity_unit_tests --gtest_filter="*Helicity*"
    DEPENDS parity_unit_tests
    COMMENT "Running Helicity-related tests"
)

# Detector-specific tests
add_custom_target(parity_test_detectors
    COMMAND parity_unit_tests --gtest_filter="*BPM*:*BCM*"
    DEPENDS parity_unit_tests
    COMMENT "Running Detector-related tests"
)

# Memory check target (if valgrind is available)
find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
    add_custom_target(parity_test_memcheck
        COMMAND ${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full --show-reachable=yes 
                --error-exitcode=1 $<TARGET_FILE:parity_unit_tests>
        DEPENDS parity_unit_tests
        COMMENT "Running Parity unit tests with memory checking"
    )
endif()

# Integration workflow test
if(TARGET parity_integration_tests)
    add_custom_target(parity_test_workflow
        COMMAND parity_integration_tests --gtest_filter="*Workflow*:*Pattern*"
        DEPENDS parity_integration_tests
        COMMENT "Running Parity analysis workflow tests"
    )
endif()

#----------------------------------------------------------------------------
# Test Installation
#
if(BUILD_TESTING)
    install(TARGETS parity_unit_tests
        RUNTIME DESTINATION bin/tests
        COMPONENT testing
    )
    
    if(TARGET parity_integration_tests)
        install(TARGETS parity_integration_tests
            RUNTIME DESTINATION bin/tests
            COMPONENT testing
        )
    endif()
    
    if(TARGET parity_benchmarks)
        install(TARGETS parity_benchmarks
            RUNTIME DESTINATION bin/benchmarks
            COMPONENT testing
        )
    endif()
endif()