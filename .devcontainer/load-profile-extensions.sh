# JAPAN-MOLLER Development Environment Profile Extensions
# This file provides convenient aliases and functions for CVMFS and LCG management

# Get LCG environment from container environment variable
LCG_ENV="${LCG_ENVIRONMENT:-LCG_107/x86_64-ubuntu2404-gcc13-opt}"
LCG_SETUP_PATH="/cvmfs/sft.cern.ch/lcg/views/${LCG_ENV}/setup.sh"

# CVMFS management aliases
alias mount-cvmfs="bash /workspaces/japan-MOLLER/.devcontainer/mount-cvmfs.sh"
alias check-cvmfs="df -h | grep cvmfs || echo 'No CVMFS mounts found. Run: mount-cvmfs'"
alias setup-lcg="source ${LCG_SETUP_PATH}"

# Function to automatically source LCG environment
auto_source_lcg() {
    # Set default LCG environment if not specified via container
    local lcg_env="${LCG_ENVIRONMENT:-LCG_107/x86_64-ubuntu2404-gcc13-opt}"

    # Only auto-source if environment not already loaded
    if [[ -z "$LCG_VERSION" && -z "$ROOTSYS" ]]; then
        local lcg_setup_path="/cvmfs/sft.cern.ch/lcg/views/$lcg_env/setup.sh"
        if [[ -f "$lcg_setup_path" ]]; then
            echo "🧪 Auto-sourcing LCG environment: $lcg_env"
            source "$lcg_setup_path"
        else
            echo "⚠️  LCG setup script not found: $lcg_setup_path"
        fi
    fi
}

# Function to check and auto-mount CVMFS on directory access
check_cvmfs_on_access() {
    if [[ "$PWD" == /cvmfs/* ]] && ! mountpoint -q /cvmfs/sft.cern.ch 2>/dev/null; then
        echo "🔧 CVMFS not mounted, attempting auto-mount..."
        bash /workspaces/japan-MOLLER/.devcontainer/mount-cvmfs.sh
        # After mounting, try to source LCG environment
        auto_source_lcg
    elif mountpoint -q /cvmfs/sft.cern.ch 2>/dev/null && [[ -z "$LCG_VIEW" ]]; then
        # CVMFS is mounted but LCG not loaded, auto-source it
        auto_source_lcg
    fi
}

# Add the check to the prompt (executed before each command)
if [[ -z "$PROMPT_COMMAND" ]]; then
    PROMPT_COMMAND="check_cvmfs_on_access"
else
    PROMPT_COMMAND="$PROMPT_COMMAND; check_cvmfs_on_access"
fi

# Auto-source LCG environment if CVMFS is available (for both interactive and non-interactive shells)
if mountpoint -q /cvmfs/sft.cern.ch 2>/dev/null; then
    auto_source_lcg
fi

# Display helpful messages on shell startup
if [[ $- == *i* ]]; then  # Only in interactive shells
    echo "🔬 JAPAN-MOLLER Development Environment"
    echo "📁 CVMFS aliases available:"
    echo "   mount-cvmfs  - Manually check and mount CVMFS repositories"
    echo "   check-cvmfs  - Show current CVMFS mount status"
    echo "   setup-lcg    - Source LCG environment ($LCG_ENV)"
    echo ""

    # Quick CVMFS status check
    if mountpoint -q /cvmfs/sft.cern.ch 2>/dev/null; then
        echo "✅ CVMFS is mounted and ready"
    else
        echo "🔄 CVMFS will be automatically mounted (postAttachCommand runs on container attach)"
        echo "🔄 LCG environment ($LCG_ENV) will be auto-loaded after CVMFS mount"
    fi
    echo ""
fi

# Auto-source LCG environment at the end (for both interactive and non-interactive shells)
if mountpoint -q /cvmfs/sft.cern.ch 2>/dev/null; then
    auto_source_lcg
fi
