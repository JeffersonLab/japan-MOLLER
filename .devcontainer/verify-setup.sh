#!/bin/bash
# Comprehensive verification script for JAPAN-MOLLER devcontainer setup

echo "🏗️  JAPAN-MOLLER Devcontainer Verification"
echo "=========================================="
echo

# 1. Test CVMFS mounting
echo "1. 📁 CVMFS Status:"
if mountpoint -q /cvmfs/sft.cern.ch; then
    echo "   ✅ CVMFS sft.cern.ch mounted"
else
    echo "   ❌ CVMFS sft.cern.ch NOT mounted"
fi

if [[ -f /cvmfs/sft.cern.ch/lcg/views/LCG_107/x86_64-ubuntu2404-gcc13-opt/setup.sh ]]; then
    echo "   ✅ LCG setup script accessible"
else
    echo "   ❌ LCG setup script NOT accessible"
fi
echo

# 2. Test LCG environment
echo "2. 🧪 LCG Environment:"
if [[ -n "$LCG_VERSION" ]]; then
    echo "   ✅ LCG_VERSION: $LCG_VERSION"
else
    echo "   ❌ LCG_VERSION not set"
fi

if [[ -n "$ROOTSYS" ]]; then
    echo "   ✅ ROOTSYS: $ROOTSYS"
else
    echo "   ❌ ROOTSYS not set"
fi

if command -v root >/dev/null 2>&1; then
    echo "   ✅ ROOT available: $(root-config --version 2>/dev/null)"
else
    echo "   ❌ ROOT not available"
fi

if command -v gcc >/dev/null 2>&1; then
    echo "   ✅ GCC available: $(gcc --version | head -1)"
else
    echo "   ❌ GCC not available"
fi
echo

# 3. Test auto-sourcing functions
echo "3. ⚙️  Shell Integration:"
if type auto_source_lcg >/dev/null 2>&1; then
    echo "   ✅ auto_source_lcg function available"
else
    echo "   ❌ auto_source_lcg function NOT available"
fi

if type mount-cvmfs >/dev/null 2>&1; then
    echo "   ✅ mount-cvmfs alias available"
else
    echo "   ❌ mount-cvmfs alias NOT available"
fi

if type check-cvmfs >/dev/null 2>&1; then
    echo "   ✅ check-cvmfs alias available"
else
    echo "   ❌ check-cvmfs alias NOT available"
fi

if type setup-lcg >/dev/null 2>&1; then
    echo "   ✅ setup-lcg alias available"
else
    echo "   ❌ setup-lcg alias NOT available"
fi
echo

# 4. Test container environment
echo "4. 🐳 Container Configuration:"
if [[ -n "$LCG_ENVIRONMENT" ]]; then
    echo "   ✅ LCG_ENVIRONMENT: $LCG_ENVIRONMENT"
else
    echo "   ⚠️  LCG_ENVIRONMENT not set (using fallback: LCG_107/x86_64-ubuntu2404-gcc13-opt)"
fi
echo

# 5. Test devcontainer scripts
echo "5. 📋 Devcontainer Scripts:"
for script in setup-cvmfs.sh post-create.sh post-attach.sh setup-profile-extensions.sh load-profile-extensions.sh; do
    if [[ -f "/workspaces/japan-MOLLER/.devcontainer/$script" ]]; then
        echo "   ✅ $script exists"
    else
        echo "   ❌ $script missing"
    fi
done
echo

# 6. Final assessment
echo "6. 📊 Overall Assessment:"
if mountpoint -q /cvmfs/sft.cern.ch && [[ -n "$LCG_VERSION" ]] && command -v root >/dev/null 2>&1; then
    echo "   🎉 SUCCESS: Devcontainer is fully configured and ready for JAPAN-MOLLER development!"
    echo "   🚀 You can now build and run physics analysis code with:"
    echo "      • CVMFS repositories mounted"
    echo "      • LCG environment automatically loaded"
    echo "      • ROOT, GCC, and physics libraries available"
else
    echo "   ⚠️  WARNING: Some components may need attention"
    echo "   💡 Try running: source ~/.bashrc && setup-lcg"
fi