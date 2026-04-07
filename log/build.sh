#!/bin/bash

# ZOO Log Module Build Script with Unity Tests
# Usage: ./build.sh [OPTIONS]

set -e  # Exit on any error

# Configuration
BUILD_TYPE="Release"
BUILD_TESTS="ON"
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SOURCE_DIR}/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_info() {
    echo -e "${BLUE}$1${NC}"
}

print_success() {
    echo -e "${GREEN}$1${NC}"
}

print_warning() {
    echo -e "${YELLOW}$1${NC}"
}

print_error() {
    echo -e "${RED}$1${NC}"
}

# Show help
show_help() {
    echo "=== Building ZOO Log Module with Unity Tests ==="
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  --no-tests    Skip building tests"
    echo "  --debug       Build in debug mode"
    echo "  --release     Build in release mode (default)"
    echo "  -h, --help    Show this help message"
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        -h|--help)
            show_help
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Build configuration summary
echo "=== Building ZOO Log Module with Unity Tests ==="
echo "Build configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Tests: $BUILD_TESTS"
echo ""

# Remove existing build directory
if [ -d "$BUILD_DIR" ]; then
    print_info "Removing existing build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
print_info "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_info "Configuring with CMake..."
cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DZOO_LOG_BUILD_TESTS="$BUILD_TESTS" \
    "$SOURCE_DIR"

# Build
print_info "Building..."
make -j$(nproc 2>/dev/null || echo 4)

print_success "=== Build Complete ==="
echo ""
echo "Library: ${BUILD_DIR}/lib/"

if [ "$BUILD_TESTS" = "ON" ]; then
    echo "Tests: ${BUILD_DIR}/bin/zoo_log_tests"
    echo ""
    echo "To run tests:"
    echo "  cd ${BUILD_DIR}/bin && ./zoo_log_tests"
    echo ""
    echo "To run with CTest:"
    echo "  cd ${BUILD_DIR} && ctest --output-on-failure"
else
    echo ""
    echo "To build tests later:"
    echo "  $0"
fi
