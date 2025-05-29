#!/bin/bash

# Setup script for Japan-MOLLER Analysis Software
# This script sets up the necessary environment variables and paths

echo "Setting up Japan-MOLLER Analysis Environment..."

# Add ROOT binaries to PATH
export PATH="/home/multivac/root_install/bin:$PATH"

# Set QWANALYSIS environment variable to point to the project directory
export QWANALYSIS="/home/multivac/japan-MOLLER"

# Set QW_PRMINPUT to point to the parameter input directory
export QW_PRMINPUT="$QWANALYSIS/Parity/prminput"

# Add the build directory to PATH so executables can be run from anywhere
export PATH="$QWANALYSIS/build:$PATH"

# Set LD_LIBRARY_PATH to include ROOT libraries and the built analysis library
export LD_LIBRARY_PATH="/home/multivac/root_install/lib:$QWANALYSIS/build:$LD_LIBRARY_PATH"

echo "Environment setup complete!"
echo "Available executables:"
echo "  - qwroot: ROOT interpreter with Qweak extensions"
echo "  - qwparity: Main parity analysis tool"
echo "  - qwmockdatagenerator: Generate mock data for testing"
echo ""
echo "To use this environment in the future, run:"
echo "  source /home/multivac/japan-MOLLER/setup_environment.sh"
echo ""
echo "Example commands:"
echo "  qwparity --help"
echo "  qwmockdatagenerator -r 4 -e 1:20000 --config qwparity_simple.conf"
