# JAPAN-MOLLER Testing and Benchmarking Framework

This document provides comprehensive information about the testing and benchmarking framework for the JAPAN-MOLLER analysis codebase.

## Overview

The testing framework provides:
- **Unit Tests**: Test individual classes and functions
- **Integration Tests**: Test complete workflows and subsystem interactions
- **Benchmarks**: Performance analysis for critical code paths
- **Coverage Analysis**: Code coverage reporting and threshold checking

## Quick Start

### 1. Build with Testing Enabled

```bash
# Configure build with testing and coverage
cmake -B build -S . -DENABLE_TESTING=ON -DENABLE_COVERAGE=ON

# Build the project
make -C build -j$(nproc)
```

### 2. Run All Tests

```bash
# Using the test runner script (recommended)
./scripts/run_tests.sh

# Or using CMake directly
cd build && ctest --output-on-failure
```

### 3. Generate Coverage Report

```bash
# Using the coverage script (recommended)
./scripts/run_coverage.sh --open

# Or using CMake directly
cd build && make coverage
```

## Test Organization

### Directory Structure

```
Analysis/tests/
├── unit/                    # Unit tests for Analysis framework
│   ├── test_QwMollerADC_Channel.cpp
│   ├── test_QwScaler_Channel.cpp
│   ├── test_QwPMT_Channel.cpp
│   ├── test_VQwDataElement.cpp
│   ├── test_VQwHardwareChannel.cpp
│   ├── test_QwEventBuffer.cpp
│   └── test_QwSubsystemArray.cpp
├── integration/             # Integration tests
│   └── test_EventProcessing.cpp
├── benchmarks/             # Performance benchmarks
│   ├── benchmark_EventProcessing.cpp
│   ├── benchmark_ChannelArithmetic.cpp
│   ├── benchmark_ROOTFileIO.cpp
│   └── benchmark_MemoryOperations.cpp
└── CMakeLists.txt

Parity/tests/
├── unit/                    # Unit tests for Parity framework
│   ├── test_QwHelicity.cpp
│   ├── test_QwBPMStripline.cpp
│   ├── test_QwBCM.cpp
│   └── test_QwBlinder.cpp
├── integration/             # Integration tests
│   └── test_HelicityAnalysis.cpp
├── benchmarks/             # Performance benchmarks
│   ├── benchmark_HelicityProcessing.cpp
│   ├── benchmark_BlindingOperations.cpp
│   └── benchmark_DetectorProcessing.cpp
└── CMakeLists.txt

scripts/
├── run_tests.sh            # Comprehensive test runner
└── run_coverage.sh         # Coverage analysis script
```

### Test Labels

Tests are labeled for easy filtering:
- `unit`: Unit tests
- `integration`: Integration tests
- `benchmark`: Performance benchmarks
- `analysis`: Analysis framework tests
- `parity`: Parity framework tests
- `quick`: Fast subset of unit tests

## Running Tests

### Using the Test Runner Script

The `scripts/run_tests.sh` script provides a convenient interface:

```bash
# Run all tests
./scripts/run_tests.sh

# Run only unit tests
./scripts/run_tests.sh --type unit

# Run tests with verbose output
./scripts/run_tests.sh --type unit --verbose

# Run specific tests by name pattern
./scripts/run_tests.sh --filter "Channel"

# Run tests multiple times to check for flakiness
./scripts/run_tests.sh --repeat 5

# Run with parallel execution
./scripts/run_tests.sh --jobs 8

# Stop on first failure
./scripts/run_tests.sh --stop-on-failure
```

### Using CTest Directly

For more control, use CTest directly:

```bash
cd build

# Run all tests
ctest --output-on-failure

# Run specific test types
ctest -L unit                    # Unit tests only
ctest -L integration            # Integration tests only
ctest -L analysis               # Analysis framework tests
ctest -L parity                 # Parity framework tests

# Run with parallel execution
ctest --parallel 4

# Run specific tests
ctest -R "Channel"              # Tests with "Channel" in name
ctest -R "test_Qw.*Channel"     # Regex pattern

# Verbose output
ctest --verbose

# Generate XML output for CI
ctest --output-junit results.xml
```

## Benchmarking

### Running Benchmarks

```bash
# Run all benchmarks
./scripts/run_tests.sh --type benchmarks

# Run specific benchmark categories
ctest -L benchmark -R "EventProcessing"
ctest -L benchmark -R "ChannelArithmetic"
ctest -L benchmark -R "ROOTFileIO"
```

### Benchmark Categories

#### Event Processing Benchmarks
- Event buffer creation and manipulation
- Event parsing and validation
- Multi-threaded event processing

#### Channel Arithmetic Benchmarks  
- Type-specific arithmetic operations
- Polymorphic arithmetic operations
- Container delegation patterns

#### ROOT File I/O Benchmarks
- File creation and writing
- Tree filling operations
- Large dataset I/O

#### Memory Benchmarks
- Memory allocation patterns
- Container operations
- Cache efficiency

### Interpreting Benchmark Results

Benchmark output includes:
- **Time**: Execution time per iteration
- **CPU**: CPU time (excluding I/O wait)
- **Iterations**: Number of iterations run
- **Bytes/sec**: Throughput for I/O operations
- **Items/sec**: Processing rate for bulk operations

## Coverage Analysis

### Using the Coverage Script

The `scripts/run_coverage.sh` script provides comprehensive coverage analysis:

```bash
# Generate full coverage report
./scripts/run_coverage.sh

# Generate coverage for specific components
./scripts/run_coverage.sh --type analysis
./scripts/run_coverage.sh --type parity
./scripts/run_coverage.sh --type unit

# Open report in browser
./scripts/run_coverage.sh --open

# Check coverage thresholds
./scripts/run_coverage.sh --threshold-check

# Clean and regenerate
./scripts/run_coverage.sh --clean

# Console output instead of HTML
./scripts/run_coverage.sh --format console
```

### Using CMake Coverage Targets

```bash
cd build

# Generate full coverage report
make coverage

# Component-specific coverage
make coverage_analysis
make coverage_parity
make coverage_unit
make coverage_integration

# Detailed coverage with branch analysis
make coverage_detailed

# CI-friendly console output
make coverage_ci

# Check coverage thresholds
make coverage_check

# Clean coverage data
make coverage_clean
```

### Coverage Thresholds

Default thresholds (configurable in `cmake/CheckCoverage.cmake`):
- **Line Coverage**: 70%
- **Function Coverage**: 60%
- **Branch Coverage**: 50%

### Reading Coverage Reports

HTML reports include:
- **Overview**: Summary statistics and threshold status
- **Directory View**: Coverage by source directory
- **File View**: Line-by-line coverage for each file
- **Function View**: Coverage by function

Color coding:
- **Green**: Lines/functions covered by tests
- **Red**: Lines/functions not covered
- **Blue**: Non-executable lines (comments, declarations)

## Writing Tests

### Unit Test Guidelines

1. **Naming Convention**: `test_<ClassName>.cpp`
2. **Test Structure**: Use Google Test framework
3. **Test Categories**: Group related tests in test suites
4. **Coverage**: Aim for high line and branch coverage

Example unit test structure:
```cpp
#include <gtest/gtest.h>
#include "Analysis/include/QwMollerADC_Channel.h"

class QwMollerADC_ChannelTest : public ::testing::Test {
protected:
    void SetUp() override {
        channel = std::make_unique<QwMollerADC_Channel>("test_channel");
    }
    
    std::unique_ptr<QwMollerADC_Channel> channel;
};

TEST_F(QwMollerADC_ChannelTest, Constructor) {
    EXPECT_EQ(channel->GetElementName(), "test_channel");
}

TEST_F(QwMollerADC_ChannelTest, ArithmeticOperations) {
    // Test type-specific operators
    // Test polymorphic operators
    // Test error conditions
}
```

### Integration Test Guidelines

1. **Workflow Testing**: Test complete analysis workflows
2. **Subsystem Interaction**: Test interactions between components
3. **Data Flow**: Test data processing pipelines
4. **Configuration**: Test with realistic configurations

### Benchmark Guidelines

1. **Realistic Data**: Use representative data sizes
2. **Multiple Scenarios**: Test best/average/worst cases
3. **Baseline Measurements**: Establish performance baselines
4. **Memory Profiling**: Include memory usage measurements

Example benchmark structure:
```cpp
#include <benchmark/benchmark.h>
#include "Analysis/include/QwEventBuffer.h"

static void BM_EventProcessing(benchmark::State& state) {
    auto buffer = std::make_unique<QwEventBuffer>();
    
    for (auto _ : state) {
        // Setup
        buffer->CreateEvent(state.range(0));
        
        // Benchmark operation
        benchmark::DoNotOptimize(buffer->ProcessEvent());
        
        // Cleanup
        buffer->ClearEvent();
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_EventProcessing)->Range(1000, 10000);
```

## Continuous Integration

### CI Test Configuration

For CI environments, use:
```bash
# Fast test subset
./scripts/run_tests.sh --type quick

# Coverage with threshold checking
./scripts/run_coverage.sh --type ci --threshold-check

# XML output for test reporting
cd build && ctest --output-junit test_results.xml
```

### Performance Regression Detection

1. **Baseline Establishment**: Run benchmarks on stable commits
2. **Regression Thresholds**: Define acceptable performance variance
3. **Automated Monitoring**: Compare benchmark results in CI

## Troubleshooting

### Common Issues

#### Tests Not Found
```bash
# Check if testing is enabled
grep "ENABLE_TESTING:BOOL=ON" build/CMakeCache.txt

# Reconfigure if needed
cmake -B build -S . -DENABLE_TESTING=ON
```

#### Coverage Not Working
```bash
# Check if coverage is enabled
grep "ENABLE_COVERAGE:BOOL=ON" build/CMakeCache.txt

# Install lcov if missing
sudo apt-get install lcov

# Reconfigure with coverage
cmake -B build -S . -DENABLE_COVERAGE=ON -DENABLE_TESTING=ON
```

#### Benchmark Failures
```bash
# Check if Google Benchmark is available
find build -name "*benchmark*"

# Install if missing (done automatically by CMake)
```

#### Permission Issues
```bash
# Make scripts executable
chmod +x scripts/run_tests.sh scripts/run_coverage.sh
```

### Performance Issues

#### Slow Tests
- Use `--jobs N` for parallel execution
- Filter tests with `-R pattern` or `--filter pattern`
- Use `--type quick` for fast subset

#### Large Coverage Reports
- Use `--type unit` or `--type integration` for focused analysis
- Use `--format console` for text output
- Filter source files in lcov commands

### Memory Issues

#### Large Memory Usage
- Limit parallel jobs with `--jobs N`
- Run test categories separately
- Use `--repeat 1` instead of multiple runs

## Advanced Usage

### Custom Test Configurations

Create custom CTest configurations:
```cmake
# In CMakeLists.txt
add_custom_target(test_custom
    COMMAND ${CMAKE_CTEST_COMMAND} -L "unit" -E "slow"
    COMMENT "Running custom test subset"
)
```

### Integration with IDEs

#### VS Code
- Install C++ Test Explorer extension
- Tests will appear in Test Explorer panel
- Coverage highlighting available with Coverage Gutters extension

#### CLion
- Tests auto-detected through CTest integration
- Built-in coverage analysis support
- Integrated benchmark runner

### Custom Coverage Filters

Modify `cmake/CheckCoverage.cmake` to customize:
- Coverage thresholds
- File filters
- Report formats

### Performance Baselines

Establish baselines for regression detection:
```bash
# Run benchmarks and save results
./scripts/run_tests.sh --type benchmarks > baseline_results.txt

# Compare with current results
./scripts/run_tests.sh --type benchmarks > current_results.txt
diff baseline_results.txt current_results.txt
```

## Best Practices

### Test Development
1. **Write tests first**: TDD approach recommended
2. **Test edge cases**: Include boundary conditions and error cases
3. **Mock dependencies**: Use mocks for external dependencies
4. **Keep tests fast**: Unit tests should run in milliseconds
5. **Make tests deterministic**: Avoid random data or timing dependencies

### Coverage Goals
1. **Aim high**: Target >80% line coverage for new code
2. **Focus on critical paths**: Ensure high coverage for core functionality
3. **Don't game metrics**: Coverage is a tool, not a goal
4. **Review uncovered code**: Understand why code isn't covered

### Performance Testing
1. **Establish baselines**: Measure before optimizing
2. **Test realistic scenarios**: Use representative data and configurations
3. **Monitor trends**: Track performance over time
4. **Profile bottlenecks**: Use detailed profiling for optimization

### Maintenance
1. **Update regularly**: Keep tests current with code changes
2. **Refactor test code**: Apply same quality standards as production code
3. **Monitor flaky tests**: Address unstable tests promptly
4. **Document test intent**: Make test purpose clear

This testing framework provides comprehensive validation and performance analysis for the JAPAN-MOLLER codebase, ensuring code quality and detecting regressions early in the development process.