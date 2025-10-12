#!/bin/bash

# Setup Profile Extensions Script for JAPAN-MOLLER Development Container
# This script configures shell integration for automatic LCG environment loading
# It adds the load-profile-extensions.sh script to the user's shell startup

set -e

echo "⚙️  Configuring shell profile extensions..."

# Ensure profile extensions are added to user's shell profile (only once)
PROFILE_MARKER="# JAPAN-MOLLER Development Environment Extensions"
if ! grep -q "$PROFILE_MARKER" ~/.bashrc 2>/dev/null; then
    echo "📝 Adding profile extensions to ~/.bashrc"
    echo "" >> ~/.bashrc
    echo "$PROFILE_MARKER" >> ~/.bashrc
    echo "if [[ -f /workspaces/japan-MOLLER/.devcontainer/load-profile-extensions.sh ]]; then" >> ~/.bashrc
    echo "    source /workspaces/japan-MOLLER/.devcontainer/load-profile-extensions.sh" >> ~/.bashrc
    echo "fi" >> ~/.bashrc
    echo "✅ Profile extensions added to ~/.bashrc"
else
    echo "✅ Profile extensions already configured in ~/.bashrc"
fi

echo "🎯 Shell integration configured for automatic LCG environment loading"
echo "🔧 LCG environment will be automatically sourced in each shell."
echo "🛠️  Convenience commands:"
echo "   - mount-cvmfs   : Check and mount CVMFS repositories"
echo "   - check-cvmfs   : Show current mount status"
echo "   - setup-lcg     : Source LCG environment"