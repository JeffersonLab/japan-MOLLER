#!/bin/bash

# JAPAN-MOLLER Coverage Analysis Script
# This script provides a convenient interface for running coverage analysis

set -e

# Default values
BUILD_DIR="build"
COVERAGE_TYPE="full"
OUTPUT_FORMAT="html"
OPEN_REPORT=false
CLEAN_FIRST=false
VERBOSE=false
THRESHOLD_CHECK=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_colored() {
    color=$1
    shift
    echo -e "${color}$*${NC}"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "JAPAN-MOLLER Coverage Analysis Script"
    echo ""
    echo "OPTIONS:"
    echo "  -t, --type TYPE          Coverage type: full, unit, integration, analysis, parity (default: full)"
    echo "  -f, --format FORMAT      Output format: html, console, json (default: html)"
    echo "  -b, --build-dir DIR      Build directory (default: build)"
    echo "  -o, --open              Open HTML report in browser after generation"
    echo "  -c, --clean             Clean coverage data before analysis"
    echo "  -v, --verbose           Verbose output"
    echo "  -T, --threshold-check   Check coverage thresholds after analysis"
    echo "  -h, --help              Show this help message"
    echo ""
    echo "EXAMPLES:"
    echo "  $0                              # Run full coverage with HTML output"
    echo "  $0 --type unit --open           # Run unit test coverage and open report"
    echo "  $0 --type analysis --format console  # Analysis framework coverage to console"
    echo "  $0 --clean --threshold-check    # Clean, run full coverage, and check thresholds"
    echo ""
    echo "COVERAGE TYPES:"
    echo "  full         - All tests (unit + integration)"
    echo "  unit         - Unit tests only"
    echo "  integration  - Integration tests only"
    echo "  analysis     - Analysis framework tests only"
    echo "  parity       - Parity framework tests only"
    echo "  detailed     - Detailed coverage with function and branch analysis"
    echo "  ci           - CI-friendly console output"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            COVERAGE_TYPE="$2"
            shift 2
            ;;
        -f|--format)
            OUTPUT_FORMAT="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -o|--open)
            OPEN_REPORT=true
            shift
            ;;
        -c|--clean)
            CLEAN_FIRST=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -T|--threshold-check)
            THRESHOLD_CHECK=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_colored $RED "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Validate coverage type
case $COVERAGE_TYPE in
    full|unit|integration|analysis|parity|detailed|ci)
        ;;
    *)
        print_colored $RED "Invalid coverage type: $COVERAGE_TYPE"
        print_colored $YELLOW "Valid types: full, unit, integration, analysis, parity, detailed, ci"
        exit 1
        ;;
esac

# Validate output format
case $OUTPUT_FORMAT in
    html|console|json)
        ;;
    *)
        print_colored $RED "Invalid output format: $OUTPUT_FORMAT"
        print_colored $YELLOW "Valid formats: html, console, json"
        exit 1
        ;;
esac

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    print_colored $RED "Build directory '$BUILD_DIR' does not exist"
    print_colored $YELLOW "Please build the project first with: cmake -B $BUILD_DIR -S . -DENABLE_COVERAGE=ON -DENABLE_TESTING=ON"
    exit 1
fi

# Check if coverage is enabled
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    print_colored $RED "CMakeCache.txt not found in $BUILD_DIR"
    exit 1
fi

if ! grep -q "ENABLE_COVERAGE:BOOL=ON" "$BUILD_DIR/CMakeCache.txt"; then
    print_colored $RED "Coverage is not enabled in the build"
    print_colored $YELLOW "Please reconfigure with: cmake -B $BUILD_DIR -S . -DENABLE_COVERAGE=ON -DENABLE_TESTING=ON"
    exit 1
fi

# Change to build directory
cd "$BUILD_DIR"

print_colored $BLUE "JAPAN-MOLLER Coverage Analysis"
print_colored $BLUE "=============================="
print_colored $BLUE "Coverage Type: $COVERAGE_TYPE"
print_colored $BLUE "Output Format: $OUTPUT_FORMAT"
print_colored $BLUE "Build Directory: $BUILD_DIR"
echo ""

# Clean coverage data if requested
if [ "$CLEAN_FIRST" = true ]; then
    print_colored $YELLOW "Cleaning coverage data..."
    make coverage_clean || {
        print_colored $RED "Failed to clean coverage data"
        exit 1
    }
    echo ""
fi

# Set verbose flag
MAKE_VERBOSE=""
if [ "$VERBOSE" = true ]; then
    MAKE_VERBOSE="VERBOSE=1"
fi

# Run coverage analysis
print_colored $YELLOW "Running coverage analysis..."

case $COVERAGE_TYPE in
    full)
        make coverage $MAKE_VERBOSE
        REPORT_DIR="coverage/html"
        ;;
    unit)
        make coverage_unit $MAKE_VERBOSE
        REPORT_DIR="coverage/html"
        ;;
    integration)
        make coverage_integration $MAKE_VERBOSE
        REPORT_DIR="coverage/html"
        ;;
    analysis)
        make coverage_analysis $MAKE_VERBOSE
        REPORT_DIR="coverage/analysis_html"
        ;;
    parity)
        make coverage_parity $MAKE_VERBOSE
        REPORT_DIR="coverage/parity_html"
        ;;
    detailed)
        make coverage_detailed $MAKE_VERBOSE
        REPORT_DIR="coverage/detailed_html"
        ;;
    ci)
        make coverage_ci $MAKE_VERBOSE
        REPORT_DIR=""
        ;;
esac

if [ $? -ne 0 ]; then
    print_colored $RED "Coverage analysis failed"
    exit 1
fi

echo ""
print_colored $GREEN "Coverage analysis completed successfully!"

# Show coverage summary
if [ -f "coverage_filtered.info" ]; then
    print_colored $BLUE "Coverage Summary:"
    lcov --summary coverage_filtered.info 2>/dev/null | grep -E "(lines|functions|branches)" || true
    echo ""
fi

# Check thresholds if requested
if [ "$THRESHOLD_CHECK" = true ]; then
    print_colored $YELLOW "Checking coverage thresholds..."
    if make coverage_check; then
        print_colored $GREEN "All coverage thresholds met!"
    else
        print_colored $RED "Coverage thresholds not met"
        exit 1
    fi
    echo ""
fi

# Display report location and open if requested
if [ -n "$REPORT_DIR" ] && [ -d "$REPORT_DIR" ]; then
    FULL_PATH="$(pwd)/$REPORT_DIR/index.html"
    print_colored $GREEN "Coverage report generated: $FULL_PATH"
    
    if [ "$OPEN_REPORT" = true ]; then
        print_colored $YELLOW "Opening coverage report..."
        
        # Detect platform and open appropriately
        if command -v xdg-open > /dev/null; then
            xdg-open "$FULL_PATH"
        elif command -v open > /dev/null; then
            open "$FULL_PATH"
        elif command -v start > /dev/null; then
            start "$FULL_PATH"
        else
            print_colored $YELLOW "Cannot auto-open browser. Please open: $FULL_PATH"
        fi
    fi
fi

print_colored $GREEN "Coverage analysis complete!"
