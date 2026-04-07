#!/bin/bash
# Build script for ZOO Platform Tests

set -e

echo "=== Building ZOO Platform Tests with Unity ==="

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    echo "Removing existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

echo "Building tests..."
make -j$(nproc)

echo "=== Build Complete ==="
echo "Test executable: $BUILD_DIR/zoo_platform_tests"
echo ""
echo "To run tests:"
echo "  cd $BUILD_DIR && ./zoo_platform_tests"
echo ""
echo "To run with RT-Thread simulation:"
echo "  cd $BUILD_DIR && cmake .. -DTEST_RTTHREAD_MODE=ON && make && ./zoo_platform_tests"
echo ""
echo "To run with coverage:"
echo "  cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON"
echo "  make -j$(nproc)"
echo "  ./zoo_platform_tests"
echo "  gcov *.gcno"
