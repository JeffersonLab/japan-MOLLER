# JAPAN-MOLLER Development Container

This directory contains the VS Code devcontainer configuration for the JAPAN-MOLLER project, providing a complete development environment with CVMFS access to CERN software repositories.

## Features

### 🛠️ **Development Tools**
- **C/C++ Development**: Complete toolchain with GCC, Clang, CMake
- **Static Analysis**: clang-tidy and clang-format pre-configured
- **Dependencies**: Boost libraries, ROOT framework
- **GitHub Integration**: GitHub CLI and Copilot extensions

### 🗂️ **CVMFS Integration**
- **Automatic Setup**: CVMFS installed and configured on container creation
- **CVMFS Repositories**: Access to 5 repositories including `sft.cern.ch`, `geant4.cern.ch`, `sw.cern.ch`, `singularity.opensciencegrid.org`, and `oasis.opensciencegrid.org`
- **LCG Software Stacks**: Pre-configured access to LCG environments
- **Persistent Mounts**: CVMFS repositories mounted on container start

### 📋 **Available Software via CVMFS**
- **ROOT**: High-energy physics data analysis framework
- **Boost**: C++ libraries for various computational tasks
- **Geant4**: Simulation toolkit for particle physics
- **Python**: Scientific computing environment
- **Compilers**: GCC, Clang with various versions

## CVMFS Repositories

The container provides access to these CVMFS repositories:

### **sft.cern.ch** - CERN Software Distribution
- LCG software stacks for particle physics
- ROOT, Geant4, and other physics software packages
- Pre-configured build environments for different architectures

### **geant4.cern.ch** - Geant4 Simulation Toolkit
- Official Geant4 installations and data files
- Multiple versions for compatibility testing
- Physics data libraries and examples

### **sw.cern.ch** - CERN Software Repository
- Additional CERN software packages
- Experimental and development tools
- Specialized physics computing software

### **singularity.opensciencegrid.org** - OSG Singularity Containers
- Pre-built container images for scientific computing
- Ready-to-use environments for various physics applications
- Containerized software stacks from OSG community

### **oasis.opensciencegrid.org** - Open Science Grid Software
- OSG software distribution
- Scientific computing tools and libraries
- Grid computing middleware and utilities

## Usage

### **Starting the Development Environment**

1. **Open in VS Code**: Click "Reopen in Container" when prompted, or use Command Palette: `Remote-Containers: Reopen in Container`

2. **Wait for Setup**: First-time setup installs CVMFS and dependencies (~5-10 minutes)

3. **Verify CVMFS**: Check that repositories are mounted:
   ```bash
   ls /cvmfs/sft.cern.ch/lcg/views/
   ```

### **Using LCG Software Environments**

Source an LCG environment to access pre-built software:
```bash
# Example: LCG 106 with GCC 13
source /cvmfs/sft.cern.ch/lcg/views/LCG_106/x86_64-el9-gcc13-opt/setup.sh

# Verify ROOT is available
root-config --version

# Build JAPAN-MOLLER with LCG environment
mkdir -p build && cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j$(nproc)
```

### **Using Additional CVMFS Resources**

```bash
# Browse CERN software repository
ls /cvmfs/sw.cern.ch/

# List available Singularity containers
ls /cvmfs/singularity.opensciencegrid.org/

# Run a Singularity container (example)
singularity exec /cvmfs/singularity.opensciencegrid.org/opensciencegrid/osgvo-el7:latest /bin/bash

# Access OSG software
ls /cvmfs/oasis.opensciencegrid.org/mis/

# Use Geant4 directly
source /cvmfs/geant4.cern.ch/geant4/11.1.0/x86_64-el9-gcc13-opt/bin/geant4.sh
```

### **Running Static Analysis**

The container includes clang-tidy with the project's `.clang-tidy` configuration:
```bash
# Analyze specific files
clang-tidy -p build Analysis/src/QwPromptSummary.cc

# Analyze multiple files
find Analysis/src -name "*.cc" | xargs -I {} clang-tidy -p build {}
```

### **GitHub Actions Locally**

Run CI workflows locally using `act`:
```bash
# List available workflows
act --list

# Run specific workflow (dry-run)
act pull_request -W .github/workflows/build-lcg-cvmfs.yml --dryrun

# Run actual workflow (requires Docker)
act pull_request -W .github/workflows/build-lcg-cvmfs.yml
```

## Configuration Files

### **devcontainer.json**
- Container base image and VS Code extensions
- Privileged mode for CVMFS FUSE mounts
- Port forwarding and user configuration

### **setup-cvmfs.sh**
- Installs CVMFS and development dependencies
- Configures CERN repository access
- Runs once during container creation

### **mount-cvmfs.sh**
- Mounts CVMFS repositories on container start
- Includes retry logic and verification
- Runs automatically when container starts

## Troubleshooting

### **CVMFS Mount Issues**
```bash
# Check CVMFS status
cvmfs_config probe

# Manually remount repositories
sudo mount -t cvmfs sft.cern.ch /cvmfs/sft.cern.ch
sudo mount -t cvmfs geant4.cern.ch /cvmfs/geant4.cern.ch
sudo mount -t cvmfs sw.cern.ch /cvmfs/sw.cern.ch
sudo mount -t cvmfs singularity.opensciencegrid.org /cvmfs/singularity.opensciencegrid.org
sudo mount -t cvmfs oasis.opensciencegrid.org /cvmfs/oasis.opensciencegrid.org

# Check mount points
df -h | grep cvmfs
```

### **Build Issues**
```bash
# Clean and rebuild
rm -rf build && mkdir build && cd build
cmake .. && make clean && make -j$(nproc)

# Check available LCG environments
ls /cvmfs/sft.cern.ch/lcg/views/ | grep LCG_10
```

### **Permission Issues**
```bash
# Fix permissions for build directory
sudo chown -R vscode:vscode /workspaces/japan-MOLLER/build
```

## Advanced Usage

### **Multiple LCG Environments**
```bash
# Switch between different LCG versions
source /cvmfs/sft.cern.ch/lcg/views/LCG_106/x86_64-el9-gcc13-opt/setup.sh
source /cvmfs/sft.cern.ch/lcg/views/LCG_107/x86_64-el9-clang16-opt/setup.sh
```

### **Custom CVMFS Configuration**
Modify `/etc/cvmfs/default.local` for additional repositories or settings:
```bash
sudo nano /etc/cvmfs/default.local
```

### **Development Workflow Integration**
The container supports the full JAPAN-MOLLER development workflow:
1. **Code editing** with IntelliSense and syntax highlighting
2. **Static analysis** with clang-tidy integration
3. **Building** with CMake and various compiler environments
4. **Testing** with mock data generation and analysis
5. **CI simulation** with local GitHub Actions execution

## Benefits

- **Consistent Environment**: Same development setup across all machines
- **CERN Software Access**: Direct access to official LCG software stacks
- **Zero Configuration**: Automatic setup of complex dependencies
- **Isolation**: No impact on host system, easy cleanup
- **Reproducibility**: Exact environment matching CI/CD pipelines
