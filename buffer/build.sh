#!/bin/bash

#******************************************************************************
# ZOO Buffer Module Build Script
# Description: Build ZOO buffer library and Unity-based tests
# Author: GitHub Copilot
# Date: 2025-08-01
#******************************************************************************

set -e

echo "=== Building ZOO Buffer Module with Unity Tests ==="

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Parse command line arguments
BUILD_TESTS=ON
BUILD_TYPE=Release

while [[ $# -gt 0 ]]; do
    case $1 in
        --no-tests)
            BUILD_TESTS=OFF
            shift
            ;;
        --debug)
            BUILD_TYPE=Debug
            shift
            ;;
        --release)
            BUILD_TYPE=Release
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --no-tests    Skip building tests"
            echo "  --debug       Build in debug mode"
            echo "  --release     Build in release mode (default)"
            echo "  -h, --help    Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "Build configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Tests: $BUILD_TESTS"
echo ""

echo "Removing existing build directory..."
rm -rf "${BUILD_DIR}"

echo "Creating build directory..."
mkdir -p "${BUILD_DIR}"

echo "Configuring with CMake..."
cd "${BUILD_DIR}"
cmake .. \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DZOO_BUFFER_BUILD_TESTS="${BUILD_TESTS}"

echo "Building..."
make -j$(nproc)

echo "=== Build Complete ==="

if [ "$BUILD_TESTS" = "ON" ]; then
    echo ""
    echo "Library: ${BUILD_DIR}/lib/"
    echo "Tests: ${BUILD_DIR}/bin/zoo_buffer_tests"
    echo ""
    echo "To run tests:"
    echo "  cd ${BUILD_DIR}/bin && ./zoo_buffer_tests"
    echo ""
    echo "To run with CTest:"
    echo "  cd ${BUILD_DIR} && ctest --output-on-failure"
else
    echo ""
    echo "Library: ${BUILD_DIR}/lib/"
    echo ""
    echo "To build tests later:"
    echo "  ${SCRIPT_DIR}/build.sh"
fi
