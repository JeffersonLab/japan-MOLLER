#!/bin/bash

# PostAttachCommand Script for JAPAN-MOLLER Development Container
# This script runs on every devcontainer attach to ensure CVMFS is mounted
# and LCG environment is available

set -e

echo "🔄 PostAttach: Ensuring CVMFS and LCG environment are ready..."

# First ensure CVMFS is mounted
/workspaces/japan-MOLLER/.devcontainer/mount-cvmfs.sh

# Ensure profile extensions are added to user's shell profile (only once)
PROFILE_MARKER="# JAPAN-MOLLER Development Environment Extensions"
if ! grep -q "$PROFILE_MARKER" ~/.bashrc 2>/dev/null; then
    echo "📝 Adding profile extensions to ~/.bashrc"
    /workspaces/japan-MOLLER/.devcontainer/setup-profile-extensions.sh
else
    echo "✅ Profile extensions already in ~/.bashrc"
fi

# Source profile extensions in current shell
if [[ -f "/workspaces/japan-MOLLER/.devcontainer/load-profile-extensions.sh" ]]; then
    source /workspaces/japan-MOLLER/.devcontainer/load-profile-extensions.sh
    echo "✅ Profile extensions loaded in current shell"
fi

echo "🎉 PostAttach complete! LCG environment will auto-load in new shells."
