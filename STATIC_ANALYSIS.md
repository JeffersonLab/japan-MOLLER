# Static Analysis Configuration

This directory contains configuration files for static analysis tools used in the JAPAN-MOLLER project.

## .clang-tidy

The `.clang-tidy` configuration file enables comprehensive static analysis using clang-tidy. It is based on the EICrecon project configuration and includes:

### Enabled Check Categories:
- **bugprone-\***: Detects potential bugs and suspicious constructs
- **concurrency-\***: Finds concurrency-related issues
- **cppcoreguidelines-\***: Enforces C++ Core Guidelines recommendations
- **modernize-\***: Suggests modern C++ best practices
- **portability-\***: Checks for portability issues
- **readability-\***: Improves code readability and maintainability

### Disabled Checks:
Some checks are disabled to avoid excessive noise or compatibility issues:
- `bugprone-easily-swappable-parameters` - Too many false positives
- `bugprone-macro-parentheses` - Conflicts with ROOT macros
- `modernize-use-trailing-return-type` - Not adopted in this codebase
- `readability-magic-numbers` - Too restrictive for physics constants
- `cppcoreguidelines-avoid-c-arrays` - C-style arrays still needed for ROOT

### Usage:

#### Prerequisites:
Install clang-tidy (version 12 or later recommended):
```bash
# Ubuntu/Debian
sudo apt-get install clang-tidy

# AlmaLinux/CentOS/RHEL
sudo dnf install clang-tools-extra
```

#### Running clang-tidy:
```bash
# Analyze a single file
clang-tidy Analysis/src/QwPromptSummary.cc

# Analyze all C++ files in a directory
find Analysis/src -name "*.cc" -exec clang-tidy {} \;

# With compilation database (recommended)
cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p . ../Analysis/src/QwPromptSummary.cc
```

#### Integration with Build System:
The configuration can be integrated into the CMake build system by adding clang-tidy as a static analyzer:
```cmake
# In CMakeLists.txt
find_program(CLANG_TIDY_COMMAND NAMES clang-tidy)
if(CLANG_TIDY_COMMAND)
    set_target_properties(target_name PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()
```

#### CI Integration:
The configuration is ready for integration with GitHub Actions or other CI systems for automated code quality checks.

### Benefits:
- **Early bug detection**: Catches potential issues before runtime
- **Code quality enforcement**: Maintains consistent coding standards
- **Modern C++ adoption**: Suggests improvements to leverage modern C++ features
- **Automated review**: Reduces manual code review overhead
- **Continuous improvement**: Helps evolve the codebase toward best practices

### Notes:
- The configuration uses `FormatStyle: file`, expecting a `.clang-format` file for formatting rules
- Analysis results should be reviewed carefully - not all suggestions may be appropriate for physics code
- Some checks may need adjustment as the codebase evolves
