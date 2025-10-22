#!/bin/bash

# PostCreateCommand Script for JAPAN-MOLLER Development Container
# This script runs once when the devcontainer is first created
# It sets up CVMFS and configures profile extensions for automatic LCG environment loading

set -e

echo "🏗️  Setting up JAPAN-MOLLER development environment..."

# First, set up CVMFS repositories
echo "📁 Setting up CVMFS repositories..."
/workspaces/japan-MOLLER/.devcontainer/setup-cvmfs.sh

# Then, configure profile extensions for automatic LCG environment loading
echo "⚙️  Setting up profile extensions..."
/workspaces/japan-MOLLER/.devcontainer/setup-profile-extensions.sh

echo "✅ Development environment setup complete!"
echo "🚀 CVMFS and LCG environment will be available on container startup."