# VS Code Debugging Setup for QwAnalysis

This document explains how to debug the QwAnalysis C++ project using Visual Studio Code.

## Prerequisites

The project requires the following dependencies:
- **CMake** (>= 3.5)
- **Boost libraries**: program_options, filesystem, system, regex
- **ROOT** (>= 6.0) - Currently installed at `/home/multivac/root_install`
- **GDB** for debugging
- **VS Code Extensions**: 
  - C/C++ extension pack
  - CMake Tools (optional but recommended)

## Project Structure

The main executables are:
- `qwparity` - Main analysis program (from `Parity/main/QwParity.cc`)
- `qwmockdatagenerator` - Mock data generator (from `Parity/main/QwMockDataGenerator.cc`)

## Build Configuration

### Manual Build Commands
```bash
# Create build directory
mkdir build
cd build

# Configure for debug build
cmake ../ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/home/multivac/root_install

# Build the project
make -j4
```

### VS Code Tasks
The following VS Code tasks are configured in `.vscode/tasks.json`:

1. **mkdir build** - Creates the build directory
2. **cmake configure** - Configures the project with CMake
3. **make** - Builds the project (default build task)
4. **make clean** - Cleans build artifacts
5. **build debug** - Configures for debug build with proper ROOT path
6. **full debug build** - Complete debug build from scratch

To run a task: `Ctrl+Shift+P` → "Tasks: Run Task" → select desired task

## Debugging Configuration

### Launch Configurations
Three debug configurations are available in `.vscode/launch.json`:

1. **Debug QwParity** - Main debug configuration
   - Uses working directory: `${workspaceFolder}/Parity/prminput`
   - Runs with exact command line arguments: `-r 4 --config qwparity_simple.conf --detectors mock_newdets.map --datahandlers mock_datahandlers.map --data . --rootfiles .`
   - Automatically builds before debugging

2. **Debug QwParity (Current Directory)** - Alternative with full paths
   - Uses working directory: `${workspaceFolder}`
   - Uses absolute paths for configuration files

3. **Debug QwParity (No Build)** - Debug without rebuilding
   - Useful for quick debug sessions when code hasn't changed

### Running the Debugger

1. **Quick Start**: Press `F5` to start debugging with the default configuration
2. **Choose Configuration**: `Ctrl+Shift+P` → "Debug: Select and Start Debugging"
3. **Set Breakpoints**: Click in the gutter next to line numbers in source files

### Command Line Equivalent
The debug configuration runs the equivalent of:
```bash
cd Parity/prminput
../build/qwparity -r 4 --config qwparity_simple.conf --detectors mock_newdets.map --datahandlers mock_datahandlers.map --data . --rootfiles .
```

## Configuration Files

The required configuration files are located in `Parity/prminput/`:
- `qwparity_simple.conf` - Main configuration
- `mock_newdets.map` - Detector mapping
- `mock_datahandlers.map` - Data handler configuration

## VS Code Settings

The project includes optimized VS Code settings in `.vscode/settings.json`:
- **IntelliSense**: Configured with proper include paths for project headers and ROOT
- **File Associations**: Proper syntax highlighting for `.conf`, `.map`, and `.in` files
- **Compiler**: Set to use g++ with C++11 standard
- **CMake Integration**: Configured to work with the build directory

## Troubleshooting

### Build Issues
1. **Missing Boost**: Install with `sudo apt install libboost-dev libboost-program-options-dev libboost-filesystem-dev libboost-system-dev libboost-regex-dev`
2. **ROOT not found**: Ensure ROOT is installed at `/home/multivac/root_install` or update `CMAKE_PREFIX_PATH`
3. **Permission issues**: Check file permissions in the build directory

### Debug Issues
1. **Symbols not found**: Ensure project is built with `CMAKE_BUILD_TYPE=Debug`
2. **Breakpoints not hit**: Check that the source file matches the compiled binary
3. **GDB errors**: Verify GDB is installed: `sudo apt install gdb`

### Performance Tips
- Use "Debug QwParity (No Build)" for faster debug starts when code hasn't changed
- Build with `-j4` (parallel build) for faster compilation
- Use `make clean` if experiencing build issues

## Example Debugging Session

1. Open the project in VS Code
2. Set a breakpoint in `Parity/main/QwParity.cc` or any analysis file
3. Press `F5` or run "Debug QwParity" configuration
4. The program will build automatically and stop at your breakpoint
5. Use the debug controls to step through code, inspect variables, etc.

## Additional Resources

- **CMake Documentation**: https://cmake.org/documentation/
- **ROOT Documentation**: https://root.cern/doc/master/
- **VS Code C++ Debugging**: https://code.visualstudio.com/docs/cpp/cpp-debug
