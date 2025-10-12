#!/bin/bash

# CVMFS Auto-Mount Script for JAPAN-MOLLER Development Container
# This script runs on every devcontainer attach to ensure CVMFS is mounted

set -e

# Log function with timestamps (only when not all mounts are present)
log() {
    echo "$(date '+%H:%M:%S') [CVMFS] $1"
}

# Function to check if a repository is mounted
is_mounted() {
    local repo=$1
    mountpoint -q "/cvmfs/$repo" 2>/dev/null
}

# Function to mount CVMFS repository with retry logic
mount_cvmfs() {
    local repo=$1
    local mount_point="/cvmfs/$repo"
    local max_attempts=3
    local attempt=1

    while [ $attempt -le $max_attempts ]; do
        if sudo mount -t cvmfs "$repo" "$mount_point" 2>/dev/null; then
            log "✅ Successfully mounted $repo"
            return 0
        else
            log "⚠️  Failed to mount $repo (attempt $attempt/$max_attempts)"
            attempt=$((attempt + 1))
            sleep 2
        fi
    done
    
    log "❌ Failed to mount $repo after $max_attempts attempts"
    return 1
}

# Repositories to mount
REPOSITORIES=("sft.cern.ch" "geant4.cern.ch" "singularity.opensciencegrid.org" "oasis.opensciencegrid.org")

# Quick check - if all repositories are mounted, exit early
all_mounted=true
for repo in "${REPOSITORIES[@]}"; do
    if ! is_mounted "$repo"; then
        all_mounted=false
        break
    fi
done

if $all_mounted; then
    # Silent success - all mounts are already available
    exit 0
fi

# If we reach here, some mounts are missing
log "🔍 Checking CVMFS mount status..."

# Ensure mount points exist
for repo in "${REPOSITORIES[@]}"; do
    sudo mkdir -p "/cvmfs/$repo"
done

# Check and mount repositories
mounted_count=0
total_repos=${#REPOSITORIES[@]}

for repo in "${REPOSITORIES[@]}"; do
    if is_mounted "$repo"; then
        mounted_count=$((mounted_count + 1))
    else
        log "📁 Mounting $repo..."
        if mount_cvmfs "$repo"; then
            mounted_count=$((mounted_count + 1))
        fi
    fi
done

if [ $mounted_count -eq $total_repos ]; then
    log "🎉 All CVMFS repositories are now available!"
else
    log "⚠️  $mounted_count/$total_repos repositories mounted. Some may require manual intervention."
fi