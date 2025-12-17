#!/bin/bash
# ==============================================================================
# Setup Script for DQN Lego Robot on Jetson AGX Xavier
# ==============================================================================
# This script installs all dependencies required to build and run the DQN
# implementation in C++ using LibTorch on the Jetson AGX Xavier.
# ==============================================================================

set -e  # Exit on error

echo "=========================================================================="
echo "  Setting up Jetson AGX Xavier for DQN Lego Robot"
echo "=========================================================================="
echo ""

# Check if running on ARM64 architecture
ARCH=$(uname -m)
if [ "$ARCH" != "aarch64" ]; then
    echo "WARNING: This script is designed for ARM64 (aarch64) architecture."
    echo "Current architecture: $ARCH"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# ==============================================================================
# 1. Update System
# ==============================================================================
echo "[1/6] Updating system packages..."
sudo apt update
sudo apt upgrade -y

# ==============================================================================
# 2. Install Development Tools
# ==============================================================================
echo "[2/6] Installing development tools..."
sudo apt install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    unzip \
    python3-pip \
    pkg-config

# ==============================================================================
# 3. Install Bluetooth Development Libraries
# ==============================================================================
echo "[3/6] Installing Bluetooth development libraries..."
sudo apt install -y \
    bluez \
    libbluetooth-dev \
    bluetooth

# Add current user to bluetooth group for permissions
echo "Adding user $USER to bluetooth group..."
sudo usermod -a -G bluetooth $USER
echo "NOTE: You may need to log out and log back in for group changes to take effect."

# ==============================================================================
# 4. Install YAML C++ Parser
# ==============================================================================
echo "[4/6] Installing yaml-cpp library..."
sudo apt install -y libyaml-cpp-dev

# ==============================================================================
# 5. Install PyTorch/LibTorch for Jetson (ARM64)
# ==============================================================================
echo "[5/6] Installing PyTorch for Jetson AGX Xavier (ARM64)..."

# Check CUDA version
if command -v nvcc &> /dev/null; then
    CUDA_VERSION=$(nvcc --version | grep "release" | sed 's/.*release //' | sed 's/,.*//')
    echo "Detected CUDA version: $CUDA_VERSION"
else
    echo "WARNING: CUDA not detected. Please install CUDA before continuing."
    exit 1
fi

LIBTORCH_DIR="/usr/local/libtorch"

# Install PyTorch from NVIDIA pre-built wheels for Jetson
if [ ! -d "$LIBTORCH_DIR" ]; then
    echo "Installing dependencies for PyTorch..."
    sudo apt install -y libopenblas-dev libopenmpi-dev

    echo "Installing PyTorch for Jetson from NVIDIA (ARM64)..."

    # Determine JetPack version and install appropriate PyTorch wheel
    # For JetPack 5.x (CUDA 11.4)
    PYTORCH_WHEEL="https://developer.download.nvidia.com/compute/redist/jp/v50/pytorch/torch-2.0.0+nv23.05-cp38-cp38-linux_aarch64.whl"

    pip3 install --no-cache-dir "$PYTORCH_WHEEL"

    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install PyTorch wheel. Trying alternative method..."
        echo "Please manually install PyTorch following NVIDIA Jetson documentation:"
        echo "https://docs.nvidia.com/deeplearning/frameworks/install-pytorch-jetson-platform/index.html"
        exit 1
    fi

    # Create symlink to LibTorch from installed PyTorch
    echo "Creating LibTorch symlink..."
    TORCH_PATH=$(python3 -c "import torch; print(torch.__path__[0])" 2>/dev/null)

    if [ -n "$TORCH_PATH" ]; then
        sudo ln -sf "$TORCH_PATH" "$LIBTORCH_DIR"
        echo "LibTorch symlink created at $LIBTORCH_DIR -> $TORCH_PATH"

        # Verify architecture
        echo "Verifying LibTorch architecture..."
        file "$LIBTORCH_DIR/lib/libtorch.so" | grep -q "aarch64"
        if [ $? -eq 0 ]; then
            echo "✓ LibTorch correctly installed for ARM64 (aarch64)"
        else
            echo "✗ WARNING: LibTorch may not be for ARM64. Check architecture with:"
            echo "  file $LIBTORCH_DIR/lib/libtorch.so"
        fi
    else
        echo "ERROR: Could not locate PyTorch installation"
        exit 1
    fi
else
    echo "LibTorch already installed at $LIBTORCH_DIR"
    echo "To reinstall, run: sudo rm -rf $LIBTORCH_DIR && ./setup_jetson.sh"
fi

# ==============================================================================
# 6. Set Environment Variables
# ==============================================================================
echo "[6/6] Setting environment variables..."

# Add LibTorch to LD_LIBRARY_PATH
if ! grep -q "LIBTORCH" ~/.bashrc; then
    echo "" >> ~/.bashrc
    echo "# LibTorch environment variables" >> ~/.bashrc
    echo "export LD_LIBRARY_PATH=$LIBTORCH_DIR/lib:\$LD_LIBRARY_PATH" >> ~/.bashrc
    echo "export CMAKE_PREFIX_PATH=$LIBTORCH_DIR:\$CMAKE_PREFIX_PATH" >> ~/.bashrc
fi

# ==============================================================================
# Verification
# ==============================================================================
echo ""
echo "=========================================================================="
echo "  Setup Complete!"
echo "=========================================================================="
echo ""
echo "Installed components:"
echo "  - Build tools (gcc, g++, cmake)"
echo "  - Bluetooth libraries (bluez, libbluetooth-dev)"
echo "  - YAML parser (libyaml-cpp-dev)"
echo "  - LibTorch ($LIBTORCH_DIR)"
echo ""
echo "Next steps:"
echo "  1. Log out and log back in (for bluetooth group permissions)"
echo "  2. Run 'source ~/.bashrc' to load environment variables"
echo "  3. Navigate to project directory and run: ./scripts/build.sh"
echo ""
echo "To verify CUDA is working with PyTorch:"
echo "  python3 -c 'import torch; print(torch.cuda.is_available())'"
echo ""
echo "=========================================================================="
