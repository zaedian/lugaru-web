#!/bin/bash
set -e

BUILD_DIR="/workspace/build-linux"
BUILD_TYPE="${1:-Release}"

echo "=== Building Lugaru for Linux ($BUILD_TYPE) ==="

if [ "$1" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    echo "Clean complete."
    exit 0
fi

cd /workspace
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX=/workspace/install-linux \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build . --config "$BUILD_TYPE" -j$(nproc)

echo ""
echo "=== Linux build complete! ==="
echo "Binary: $BUILD_DIR/lugaru"
