# RNTuple Conditional Compilation Implementation

## Summary
This implementation adds conditional compilation support for ROOT RNTuple functionality, allowing the codebase to compile with older ROOT versions (< 6.36) that don't support RNTuples.

## Changes Made

### 1. CMakeLists.txt (Main)
- Added ROOT version detection (6.36+ required for RNTuple)
- Automatically defines `HAS_RNTUPLE_SUPPORT` preprocessor flag when ROOT >= 6.36.0
- Displays informative messages about RNTuple support status

### 2. Analysis/include/QwRootFile.h
- Wrapped all RNTuple-related headers in `#ifdef HAS_RNTUPLE_SUPPORT`
- Conditionally compiled the entire `QwRootNTuple` class
- Wrapped RNTuple method declarations in `QwRootFile` class
- Wrapped RNTuple member variables (fNTupleByName, fNTupleByAddr, etc.)
- Wrapped RNTuple method implementations
- Wrapped RNTuple cleanup code in destructor

### 3. Parity/main/QwParity.cc
- Conditionally included RNTuple headers

### 4. panguin/CMakeLists.txt
- Added ROOT version detection for panguin
- Defines `HAS_RNTUPLE_SUPPORT` when appropriate

### 5. panguin/include/panguinOnline.hh
- Conditionally included RNTuple headers
- Wrapped RNTuple member variables:
  - `fRootNTuple`
  - `fRootNTupleNames` 
  - `fNTupleEntries`
  - `ntupleVars`
- Wrapped RNTuple method declarations

### 6. panguin/src/panguinOnline.cc
- Wrapped RNTuple function implementations:
  - `GetRootNTuple()`
  - `GetNTupleVars()`
  - `GetNTupleIndex()`
  - `NTupleDraw()`
- Conditionally compiled RNTuple calls in `DoDraw()` and `OpenRootFile()`

## How It Works

### With ROOT >= 6.36 (RNTuple Supported)
- `HAS_RNTUPLE_SUPPORT` is defined
- All RNTuple functionality is compiled and available
- Full compatibility with new RNTuple-based analysis workflows

### With ROOT < 6.36 (RNTuple Not Supported)
- `HAS_RNTUPLE_SUPPORT` is not defined
- RNTuple code is excluded from compilation
- Project builds successfully with traditional TTree-only functionality
- No RNTuple dependencies or namespace references

## Build Instructions

```bash
mkdir build
cd build
cmake ../
make
```

The build system will automatically:
1. Detect your ROOT version
2. Enable/disable RNTuple support accordingly
3. Display the status during configuration
4. Compile only the appropriate code paths

## Verification

The implementation has been tested with:
- ✅ ROOT 6.36.000 (RNTuple supported) - Full functionality
- ✅ Conditional compilation test - Compiles both with and without RNTuple support
- ✅ Build system integration - Automatic detection and flag setting

## Benefits

1. **Backward Compatibility**: Works with older ROOT installations
2. **Forward Compatibility**: Takes full advantage of RNTuple when available
3. **Automatic Detection**: No manual configuration required
4. **Clean Code**: RNTuple code is cleanly separated but not duplicated
5. **Performance**: No runtime overhead - decisions made at compile time

## Future Maintenance

When adding new RNTuple functionality:
1. Wrap new RNTuple code in `#ifdef HAS_RNTUPLE_SUPPORT` blocks
2. Provide TTree-based alternatives where appropriate
3. Update method signatures to be conditionally available
4. Test compilation with both ROOT versions when possible
