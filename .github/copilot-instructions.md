# JAPAN-MOLLER Analysis Framework

**Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.**

## Working Effectively

JAPAN (Just Another Parity ANalyzer) is a C++ physics data analysis framework for the MOLLER experiment. The project uses CMake and has complex ROOT/Boost dependencies that require specific setup procedures.

### Critical Build Requirements - NEVER CANCEL BUILDS

- **NEVER CANCEL ANY BUILD OR LONG-RUNNING COMMAND** - Docker builds take 15-45 minutes, compilation takes 5-15 minutes
- **Always use timeout values of 60+ minutes for builds and 30+ minutes for tests**
- **If a command appears to hang, wait at least 60 minutes before considering alternatives**

### Primary Build Method: Docker (RECOMMENDED)

Use the Docker containerized build as the primary approach. The host environment dependency management is complex and fragile.

```bash
# Build Docker image - takes 15-45 minutes. NEVER CANCEL. Set timeout to 60+ minutes.
docker build -t japan-moller .

# Run analysis in container
docker run -it japan-moller /bin/bash
```

Docker build includes all dependencies (ROOT, Boost, MySQL++) pre-configured on AlmaLinux 9.

### Alternative: Host Environment Build (COMPLEX)

Only attempt if Docker is unavailable. Requires significant dependency management.

#### Install Dependencies
```bash
# Ubuntu/Debian - ROOT installation is challenging, try multiple approaches:
# Option 1: Try conda/mamba
conda install -c conda-forge root boost-cpp

# Option 2: Build from source (takes 60+ minutes, NEVER CANCEL)
# Download ROOT source and follow build instructions

# Always install system dependencies:
sudo apt-get install -y build-essential cmake libboost-all-dev libmysqlclient-dev
```

#### Environment Setup
```bash
# Generate environment setup scripts
./SetupFiles/make_SET_ME_UP

# Source the generated environment - REQUIRED before every build/run
source SetupFiles/SET_ME_UP.bash

# Create required directories
mkdir -p /path/to/scratch/directory
export QWSCRATCH=/path/to/scratch/directory
```

#### Build Process - NEVER CANCEL
```bash
# Configure build - takes 1-3 minutes
mkdir -p build
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/root

# Compile - takes 5-15 minutes. NEVER CANCEL. Set timeout to 30+ minutes.
make -C build -j$(nproc)

# Install (optional)
make -C build install
```

### Testing and Validation

**Always run the complete test suite after any changes**

```bash
# Run all regression tests - takes 5-10 minutes. NEVER CANCEL. Set timeout to 20+ minutes.
./Tests/run_tests.sh

# Individual tests (require completed build first):
./Tests/001_compilation.sh    # Build test (may use different build system)
./Tests/002_setupfiles.sh     # Environment test  
./Tests/003_qwanalysis.sh     # Basic analysis
./Tests/004_qwparity.sh       # Mock data test

# Note: Test framework may expect legacy build system - adapt as needed
```

### Validation Scenarios - ALWAYS TEST THESE

After making changes, always test these complete workflows:

1. **Mock Data Generation and Analysis**:
   ```bash
   # Generate mock data - takes 1-2 minutes
   build/qwmockdatagenerator -r 10 -e :10000 --config qwparity_simple.conf --detectors mock_detectors.map
   
   # Analyze mock data - takes 2-5 minutes  
   build/qwparity -r 10 -e :10000 --config qwparity_simple.conf --detectors mock_detectors.map
   ```

2. **Panguin Plotting Tool**:
   ```bash
   # Build panguin separately
   cd panguin && mkdir -p build
   cmake -B build -S . && make -C build
   
   # Test basic plotting functionality
   ./build/panguin -f macros/default.cfg
   ```

3. **Environment Validation**:
   ```bash
   # Verify all environment variables are set
   echo $QWANALYSIS $QWSCRATCH $ROOTSYS
   
   # Test ROOT functionality
   root-config --version
   root -l -q -e "gROOT->GetVersion()"
   ```

## Project Structure and Navigation

### Key Directories
- **Analysis/** - Core analysis engine classes and framework
- **Parity/** - Main analysis executables and configurations
  - `Parity/main/` - Source for qwparity and qwmockdatagenerator executables
  - `Parity/prminput/` - Configuration files (*.conf) and detector maps (*.map)
- **panguin/** - Standalone plotting/visualization tool with separate build
- **Tests/** - Regression test framework
- **SetupFiles/** - Environment configuration scripts  
- **evio/** - CODA event I/O library (auto-downloaded during build)
- **cmake/** - Build system configuration

### Main Executables
- **qwparity** - Primary parity analysis executable
- **qwmockdatagenerator** - Mock data generator for testing
- **panguin** - Plotting and visualization tool

### Configuration Files
- `Parity/prminput/prex*.conf` - Main analysis configurations
- `Parity/prminput/qwparity_simple.conf` - Simple configuration for testing
- `Parity/prminput/mock_detectors.map` - Mock detector mappings for testing
- `Parity/prminput/*.map` - Detector and data handler mappings  
- `panguin/macros/*.cfg` - Plotting configuration files

## Common Issues and Solutions

### Build Failures
- **ROOT not found**: Ensure ROOT is properly installed and ROOTSYS is set. Use Docker approach if ROOT installation is problematic
- **Boost library errors**: Install boost-devel packages, not just boost runtime
- **MySQL++ missing**: Database support is optional, build will proceed without it
- **Compiler errors**: Ensure C++11 or later compiler (GCC 4.8+)
- **features.h not found**: Mixed conda/system library conflict - use pure Docker or pure system approach

### Environment Issues  
- **QWSCRATCH errors**: Create directory and set environment variable before sourcing setup
- **Path conflicts**: Use clean environment, avoid mixing conda and system libraries
- **Permission errors**: Ensure write access to build directories

### Runtime Issues
- **Missing configuration files**: Configuration files are in Parity/prminput/ directory
- **ROOT file errors**: Ensure input data files exist and are accessible
- **Memory issues**: Large datasets may require significant RAM (8GB+ recommended)

## Timing Expectations

- **Docker build**: 15-45 minutes (includes dependency compilation)
- **CMake configuration**: 1-3 minutes  
- **Compilation (make)**: 5-15 minutes with parallel build
- **Full test suite**: 5-10 minutes
- **Mock data generation**: 1-2 minutes for small datasets
- **Analysis runs**: 2-5 minutes for mock data, varies for real data

## Development Workflow

1. **Always start with environment setup**:
   ```bash
   source SetupFiles/SET_ME_UP.bash
   ```

2. **Use existing test framework for validation**:
   ```bash
   ./Tests/run_tests.sh
   ```

3. **For panguin changes, test both CLI and GUI modes** (note: GUI may not work in headless environments)

4. **Always test with mock data before using real data**

5. **Check configuration file syntax** if analysis fails - files are in Parity/prminput/

## Critical Notes

- **Database support is optional** - MySQL++ failures will not prevent build
- **Mac builds have known issues** - use Docker on macOS  
- **Large memory requirements** - ensure adequate RAM for data processing
- **Environment variables are critical** - always source setup scripts
- **Configuration files are required** - analysis will fail without proper *.conf and *.map files

**Remember: Always wait for long-running commands to complete. Builds may take 45+ minutes in some environments.**

## Common Commands and Frequently Used Information

The following are outputs from frequently run commands. Reference them instead of running bash commands to save time.

### Repository Root Structure
```
ls -la
Analysis/          # Core analysis framework
CMakeLists.txt     # Main build configuration
Dockerfile         # Container build definition  
Parity/            # Main analysis executables and configs
README.md          # Basic project documentation
SetupFiles/        # Environment setup scripts
Tests/             # Test framework
bin/               # Built executables (after make install)
build/             # Build directory (after mkdir build)
cmake/             # Build system configuration
evio/              # CODA event I/O library
lib/               # Built libraries (after make install)
panguin/           # Plotting tool (separate build)
```

### Key Executable Locations
```
# After successful build:
build/qwparity                    # Main analysis executable
build/qwmockdatagenerator        # Mock data generator
panguin/build/panguin            # Plotting tool (requires separate build)
```

### Essential Configuration Files
```
# Main analysis configurations:
Parity/prminput/prex.conf              # Main PREX analysis config
Parity/prminput/qwparity_simple.conf   # Simple test configuration
Parity/prminput/mock_detectors.map     # Mock detector mapping for tests

# Panguin plotting configurations:
panguin/macros/default.cfg             # Default plotting configuration
panguin/macros/defaultOnline.cfg       # Online monitoring configuration
```

### Environment Variables After Setup
```
QWANALYSIS=/path/to/japan-MOLLER       # Project root directory  
QWSCRATCH=/path/to/scratch             # Scratch/temporary directory
ROOTSYS=/path/to/root                  # ROOT installation directory
BOOST_INC_DIR=/usr/include             # Boost headers location
BOOST_LIB_DIR=/usr/lib/x86_64-linux-gnu # Boost libraries location
```

### Dual Design Pattern Architecture

JAPAN-MOLLER implements **two distinct architectural patterns** for arithmetic operations, each optimized for different abstraction levels:

#### **Pattern 1: Dual-Operator Pattern (Channel/Data Element Level)**

**Used by**: `VQwDataElement`, `VQwHardwareChannel`, and all concrete channel classes (`QwVQWK_Channel`, `QwMollerADC_Channel`, etc.)

**When to use**: For individual data elements requiring inheritance-based polymorphism

**Key Characteristics:**
- **Two operator versions**: Type-specific + polymorphic overloads
- **Complex inheritance**: Requires virtual method overrides and dynamic_cast
- **Runtime type safety**: Uses dynamic_cast with exception handling
- **Dual dispatch**: Polymorphic operators delegate to type-specific ones

**Implementation Pattern:**
```cpp
// Type-specific version (efficient, direct)
QwVQWK_Channel& operator+= (const QwVQWK_Channel &value);

// Polymorphic version (for inheritance compatibility)  
VQwHardwareChannel& operator+=(const VQwHardwareChannel& input) override;

// Polymorphic implementation delegates to type-specific
VQwHardwareChannel& QwVQWK_Channel::operator+=(const VQwHardwareChannel &source) {
  const QwVQWK_Channel* tmpptr = dynamic_cast<const QwVQWK_Channel*>(&source);
  if (tmpptr != NULL) {
    *this += *tmpptr;  // Calls type-specific version
  } else {
    throw std::invalid_argument("Type mismatch in operator+=");
  }
  return *this;
}

// Sum method uses assignment + operators pattern
void Sum(const QwVQWK_Channel &value1, const QwVQWK_Channel &value2) {
  *this = value1;      // Uses derived class assignment operator
  *this += value2;     // Uses type-specific operator+=
}
```

**Base Class Design Principles:**
- Virtual operators should be **non-virtual** (serve as error handlers only)
- Virtual `Sum()` and `Difference()` methods provide polymorphic interface
- Base class provides fallback implementations that throw runtime errors for unimplemented operations

#### **Pattern 2: Container-Delegation Pattern (Array/System Level)**

**Used by**: `QwSubsystemArrayParity`, `QwSubsystemArray`, and other container classes

**When to use**: For collections of heterogeneous objects requiring system-level operations

**Key Characteristics:**
- **Single operator version**: Only type-specific operators needed
- **Container iteration**: Delegates to contained object operators
- **Type checking via typeid**: Uses `typeid` comparison for safety
- **No inheritance conflicts**: Avoids virtual operator issues

**Implementation Pattern:**
```cpp
// Only one operator version needed
QwSubsystemArrayParity& operator+= (const QwSubsystemArrayParity &value) {
  for(size_t i=0; i<value.size(); i++){
    VQwSubsystemParity *ptr1 = dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
    VQwSubsystem *ptr2 = value.at(i).get();
    if (typeid(*ptr1)==typeid(*ptr2)){
      *(ptr1) += ptr2;  // Delegates to subsystem operators
    } else {
      // Handle type mismatch
    }
  }
  return *this;
}

// Sum method follows same assignment + operators pattern
void Sum(const QwSubsystemArrayParity &value1, const QwSubsystemArrayParity &value2) {
  *this = value1;
  *this += value2;
}
```

**Container Design Principles:**
- **No virtual operators** in base container classes
- **Composition over inheritance**: Use contained objects for polymorphism
- **Delegation pattern**: Let individual elements handle their own arithmetic
- **Type safety via runtime checks**: Use `typeid` and `dynamic_cast`

#### **Pattern Selection Guidelines**

**Use Dual-Operator Pattern when:**
- Implementing individual data element classes
- Need inheritance-based polymorphism
- Working with channel-level operations
- Extending `VQwDataElement` or `VQwHardwareChannel`
- Require both type-safe and polymorphic operations

**Use Container-Delegation Pattern when:**
- Implementing collection/array classes
- Managing heterogeneous object containers
- Working with system-level operations
- Extending `QwSubsystemArray` or similar containers
- Want to avoid virtual operator inheritance issues

#### **Compilation Warning Prevention**

**For Dual-Operator Pattern:**
- Remove `virtual` from base class operators that serve as defaults
- Implement both type-specific and polymorphic operator versions
- Use `override` keyword for virtual method overrides
- Use `using` declarations when needed to bring base functions into scope

**For Container-Delegation Pattern:**
- Avoid virtual operators in container base classes
- Use container iteration with type checking
- Delegate arithmetic to contained objects
- No special inheritance handling needed

#### **Key Architectural Insight**

The **two patterns complement each other**:
- **Dual-Operator Pattern**: Handles complex inheritance at the individual object level
- **Container-Delegation Pattern**: Manages collections without inheritance complexity

This dual approach provides:
- **Performance optimization**: Type-specific operations when possible
- **Type safety**: Runtime checking with meaningful error messages
- **Flexibility**: Support for both homogeneous and heterogeneous operations
- **Maintainability**: Clear separation of concerns between object and container levels

**Critical Understanding**: The choice of pattern depends on the **abstraction level** - use dual-operator for individual objects requiring inheritance polymorphism, and container-delegation for collections that can delegate to their elements.
