# Code Coverage Analysis for JAPAN-MOLLER

This document describes how to use the code coverage analysis features in the JAPAN-MOLLER testing framework.

## Prerequisites

Before using coverage analysis, ensure you have the required tools installed:

```bash
# Ubuntu/Debian
sudo apt install lcov gcov

# RHEL/CentOS/Fedora
sudo yum install lcov gcc
# or
sudo dnf install lcov gcc

# macOS (with Homebrew)
brew install lcov
```

## Building with Coverage

To enable coverage analysis, configure the build with the `ENABLE_COVERAGE` option:

```bash
# Configure with coverage enabled
cmake -B build -S . -DENABLE_COVERAGE=ON -DENABLE_TESTING=ON

# Build the project
cmake --build build
```

## Coverage Targets

The following coverage targets are available:

### Basic Coverage Analysis

```bash
# Run full coverage analysis (all tests)
make coverage

# Generate coverage summary (console output)
make coverage_summary

# Clean coverage data
make coverage_clean
```

### Test-Specific Coverage

```bash
# Unit tests only
make coverage_unit

# Integration tests only
make coverage_integration

# Analysis framework tests only
make coverage_analysis

# Parity framework tests only
make coverage_parity
```

### Advanced Coverage

```bash
# Detailed coverage with function and branch analysis
make coverage_detailed

# CI-friendly coverage (console output only)
make coverage_ci

# Check coverage thresholds
make coverage_check
```

## Coverage Reports

Coverage reports are generated in the `build/coverage/` directory:

- `build/coverage/html/index.html` - Main coverage report
- `build/coverage/analysis_html/` - Analysis framework coverage
- `build/coverage/parity_html/` - Parity framework coverage
- `build/coverage/detailed_html/` - Detailed coverage with function/branch data

## Coverage Thresholds

The following minimum coverage thresholds are enforced:

- **Line Coverage**: 70%
- **Function Coverage**: 60%
- **Branch Coverage**: 50%

These thresholds can be modified in `cmake/CheckCoverage.cmake`.

## Usage Examples

### Complete Coverage Workflow

```bash
# 1. Configure with coverage
cmake -B build -S . -DENABLE_COVERAGE=ON -DENABLE_TESTING=ON

# 2. Build the project
cmake --build build

# 3. Run coverage analysis
cd build
make coverage

# 4. View results
open coverage/html/index.html  # macOS
# or
xdg-open coverage/html/index.html  # Linux
```

### Continuous Integration

```bash
# For CI pipelines (console output only)
make coverage_ci

# Check if coverage meets thresholds
make coverage_check
```

### Development Workflow

```bash
# Quick unit test coverage
make coverage_unit

# Check specific framework
make coverage_analysis  # or coverage_parity

# Clean and re-run
make coverage_clean
make coverage
```

## Coverage Exclusions

The following files/directories are excluded from coverage analysis:

- `/usr/*` - System headers
- `*/thirdparty/*` - Third-party libraries
- `*/evio/*` - EVIO library
- `*Dict.cxx` - ROOT dictionary files
- `*/tests/*` - Test files themselves
- `*test*` - Test-related files
- `*benchmark*` - Benchmark files

## Troubleshooting

### "gcov: command not found"

Install the GCC compiler suite:

```bash
sudo apt install gcc  # Ubuntu/Debian
sudo yum install gcc  # RHEL/CentOS
```

### "lcov: command not found"

Install lcov:

```bash
sudo apt install lcov  # Ubuntu/Debian
sudo yum install lcov  # RHEL/CentOS
```

### Low Coverage Numbers

1. Ensure tests are actually running:
   ```bash
   make test
   ```

2. Check that coverage flags are applied:
   ```bash
   make VERBOSE=1
   ```

3. Verify test execution:
   ```bash
   ctest --verbose
   ```

### No Coverage Data

1. Clean and rebuild with coverage:
   ```bash
   make coverage_clean
   make coverage
   ```

2. Check for `.gcda` files:
   ```bash
   find . -name "*.gcda"
   ```

### Coverage Report Not Generated

1. Check lcov version (requires 1.13+):
   ```bash
   lcov --version
   ```

2. Verify HTML generation:
   ```bash
   genhtml --version
   ```

## Integration with IDEs

### Visual Studio Code

Install the "Coverage Gutters" extension and configure it to read lcov files:

1. Install "Coverage Gutters" extension
2. Open `build/coverage_filtered.info`
3. Enable coverage display with `Ctrl+Shift+P` > "Coverage Gutters: Display Coverage"

### CLion

CLion has built-in support for coverage analysis:

1. Configure build with coverage flags
2. Run tests with coverage: "Run with Coverage"
3. View coverage in the editor

## Advanced Configuration

### Custom Thresholds

Modify `cmake/CheckCoverage.cmake` to adjust coverage thresholds:

```cmake
set(MIN_LINE_COVERAGE 80)     # Increase line coverage requirement
set(MIN_FUNCTION_COVERAGE 70) # Increase function coverage requirement
set(MIN_BRANCH_COVERAGE 60)   # Increase branch coverage requirement
```

### Additional Exclusions

Add patterns to exclude from coverage in the main `CMakeLists.txt`:

```cmake
COMMAND ${LCOV_EXECUTABLE} --remove coverage_filtered.info '*/your_pattern/*' --output-file coverage_filtered.info
```

### Custom Reports

Create custom coverage reports for specific modules:

```bash
# Example: Coverage for specific subdirectory
lcov --capture --directory . --output-file custom.info
lcov --extract custom.info '*/YourModule/*' --output-file custom_filtered.info
genhtml custom_filtered.info --output-directory custom_coverage
```
