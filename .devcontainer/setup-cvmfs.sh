#!/bin/bash

# JAPAN-MOLLER Development Container Setup Script
# This script installs CVMFS and development dependencies

set -e

echo "🚀 Setting up JAPAN-MOLLER development environment..."

# Update package list
sudo apt-get update

echo "📦 Installing system dependencies..."
sudo apt-get install -y \
    build-essential \
    cmake \
    gcc \
    g++ \
    git \
    wget \
    curl \
    vim \
    clang-tidy \
    clang-format \
    libboost-all-dev \
    libxcb-shm0 \
    libxcb-render0 \
    libxpm4 \
    libxrender1 \
    python3 \
    python3-pip

echo "🗂️  Installing CVMFS..."

# Install CVMFS repository configuration
wget -q https://ecsft.cern.ch/dist/cvmfs/cvmfs-release/cvmfs-release-latest_all.deb
sudo dpkg -i cvmfs-release-latest_all.deb
sudo apt-get update

# Install CVMFS
sudo apt-get install -y cvmfs cvmfs-config-default

# Create CVMFS configuration
sudo mkdir -p /etc/cvmfs

# Configure CVMFS for CERN and OpenScienceGrid repositories
sudo tee /etc/cvmfs/default.local > /dev/null << EOF
CVMFS_REPOSITORIES=sft.cern.ch,geant4.cern.ch,singularity.opensciencegrid.org,oasis.opensciencegrid.org,sw.cern.ch
CVMFS_HTTP_PROXY=DIRECT
CVMFS_QUOTA_LIMIT=8000
CVMFS_CACHE_BASE=/tmp/cvmfs-cache
EOF

# Create CVMFS mount points
sudo mkdir -p /cvmfs/sft.cern.ch /cvmfs/geant4.cern.ch /cvmfs/singularity.opensciencegrid.org /cvmfs/oasis.opensciencegrid.org /cvmfs/sw.cern.ch

# Create CVMFS cache directory
sudo mkdir -p /tmp/cvmfs-cache
sudo chmod 755 /tmp/cvmfs-cache

# Set up autofs for CVMFS (if available)
if systemctl is-active --quiet autofs 2>/dev/null; then
    echo "/cvmfs /etc/auto.cvmfs" | sudo tee -a /etc/auto.master
    sudo systemctl reload autofs
fi

echo "🔧 Installing additional development tools..."

# Install act for running GitHub Actions locally
curl -q https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash
sudo mv ./bin/act /usr/local/bin/ 2>/dev/null || true

# Clean up
rm -f cvmfs-release-latest_all.deb
sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*

echo "✅ Setup complete! CVMFS will be mounted on container start."
echo "📖 Available repositories:"
echo "   - LCG software stacks: /cvmfs/sft.cern.ch/lcg/views/"
echo "   - Geant4 toolkit: /cvmfs/geant4.cern.ch/"
echo "   - CERN software: /cvmfs/sw.cern.ch/"
echo "   - OSG Singularity containers: /cvmfs/singularity.opensciencegrid.org/"
echo "   - OSG OASIS software: /cvmfs/oasis.opensciencegrid.org/"
