#!/bin/bash
set -e

BUILD_TYPE="${1:-Release}"

echo "=== Building Lugaru for all platforms ($BUILD_TYPE) ==="
echo ""

if [ "$1" = "clean" ]; then
    echo "Cleaning all build directories..."
    rm -rf /workspace/build-linux
    rm -rf /workspace/build-win64
    rm -rf /workspace/build-win32
    echo "Clean complete."
    exit 0
fi

bash /workspace/.devcontainer/build-linux.sh "$BUILD_TYPE"
echo ""

bash /workspace/.devcontainer/build-windows-64.sh "$BUILD_TYPE"
echo ""

bash /workspace/.devcontainer/build-windows-32.sh "$BUILD_TYPE"
echo ""

echo "=== All builds complete! ==="
