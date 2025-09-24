#!/bin/bash

# CVMFS Mount Script for JAPAN-MOLLER Development Container
# This script mounts CVMFS repositories on container start

set -e

echo "🗂️  Mounting CVMFS repositories..."

# Function to mount CVMFS repository with retry logic
mount_cvmfs() {
    local repo=$1
    local mount_point="/cvmfs/$repo"
    local max_attempts=3
    local attempt=1

    while [ $attempt -le $max_attempts ]; do
        echo "📁 Mounting $repo (attempt $attempt/$max_attempts)..."
        
        if sudo mount -t cvmfs "$repo" "$mount_point" 2>/dev/null; then
            echo "✅ Successfully mounted $repo"
            return 0
        else
            echo "⚠️  Failed to mount $repo (attempt $attempt/$max_attempts)"
            attempt=$((attempt + 1))
            sleep 2
        fi
    done
    
    echo "❌ Failed to mount $repo after $max_attempts attempts"
    return 1
}

# Ensure mount points exist
sudo mkdir -p /cvmfs/sft.cern.ch /cvmfs/geant4.cern.ch /cvmfs/singularity.opensciencegrid.org /cvmfs/oasis.opensciencegrid.org /cvmfs/sw.cern.ch

# Mount CERN SFT repository (contains LCG software stacks)
if ! mountpoint -q /cvmfs/sft.cern.ch; then
    mount_cvmfs "sft.cern.ch"
fi

# Mount Geant4 repository
if ! mountpoint -q /cvmfs/geant4.cern.ch; then
    mount_cvmfs "geant4.cern.ch"
fi

# Mount OpenScienceGrid Singularity repository
if ! mountpoint -q /cvmfs/singularity.opensciencegrid.org; then
    mount_cvmfs "singularity.opensciencegrid.org"
fi

# Mount OpenScienceGrid OASIS repository
if ! mountpoint -q /cvmfs/oasis.opensciencegrid.org; then
    mount_cvmfs "oasis.opensciencegrid.org"
fi

# Verify mounts and show available software
echo "📋 Verifying CVMFS mounts..."

if mountpoint -q /cvmfs/sft.cern.ch; then
    echo "✅ sft.cern.ch mounted successfully"
    echo "📊 Available LCG versions:"
    ls /cvmfs/sft.cern.ch/lcg/views/ | grep -E "LCG_10[67]" | head -5
else
    echo "❌ sft.cern.ch not mounted"
fi

if mountpoint -q /cvmfs/geant4.cern.ch; then
    echo "✅ geant4.cern.ch mounted successfully"
else
    echo "❌ geant4.cern.ch not mounted"
fi

if mountpoint -q /cvmfs/singularity.opensciencegrid.org; then
    echo "✅ singularity.opensciencegrid.org mounted successfully"
    echo "🐳 Available Singularity containers:"
    ls /cvmfs/singularity.opensciencegrid.org/ 2>/dev/null | head -3 || echo "   (will be populated on first access)"
else
    echo "❌ singularity.opensciencegrid.org not mounted"
fi

if mountpoint -q /cvmfs/oasis.opensciencegrid.org; then
    echo "✅ oasis.opensciencegrid.org mounted successfully"
    echo "📦 Available OSG software:"
    ls /cvmfs/oasis.opensciencegrid.org/ 2>/dev/null | head -3 || echo "   (will be populated on first access)"
else
    echo "❌ oasis.opensciencegrid.org not mounted"
fi

echo "🎉 CVMFS setup complete!"
echo "💡 Usage examples:"
echo "   LCG environment: source /cvmfs/sft.cern.ch/lcg/views/LCG_XXX/x86_64-*-*-opt/setup.sh"
echo "   Singularity: singularity exec /cvmfs/singularity.opensciencegrid.org/..."
echo "   OSG software: ls /cvmfs/oasis.opensciencegrid.org/"