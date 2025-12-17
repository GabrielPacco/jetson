#!/bin/bash
# ==============================================================================
# Build Script for DQN Lego Robot
# ==============================================================================

set -e  # Exit on error

PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
BUILD_DIR="${PROJECT_DIR}/build"

echo "=========================================================================="
echo "  Building DQN Lego Robot"
echo "=========================================================================="
echo "Project directory: $PROJECT_DIR"
echo "Build directory: $BUILD_DIR"
echo ""

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure with CMake
echo "[1/3] Configuring with CMake..."
cmake -DCMAKE_PREFIX_PATH=/usr/local/libtorch \
      -DCMAKE_BUILD_TYPE=Release \
      ..

# Build
echo "[2/3] Building..."
make -j$(nproc)

# Run tests
echo "[3/3] Running tests..."
ctest --output-on-failure || echo "Some tests failed"

echo ""
echo "=========================================================================="
echo "  Build Complete!"
echo "=========================================================================="
echo ""
echo "Executables in ${BUILD_DIR}:"
echo "  - train              : Training application"
echo "  - inference          : Inference application"
echo "  - test_bluetooth     : Bluetooth testing utility"
echo ""
echo "To run training:"
echo "  cd ${BUILD_DIR} && ./train"
echo ""
echo "To run inference:"
echo "  cd ${BUILD_DIR} && ./inference models/dqn_best.pt"
echo ""
echo "=========================================================================="
