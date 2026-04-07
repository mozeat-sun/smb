#!/bin/bash
################################################################################
# ZOO Thread Pool Module Build Script
# Supports Unity testing framework
################################################################################

set -e

BUILD_DIR="/home/sky/zoo/thread_pool/build"
SOURCE_DIR="/home/sky/zoo/thread_pool"

# Default values
BUILD_TYPE="Debug"
BUILD_TESTS="OFF"
RUN_TESTS=false
CLEAN_BUILD=false
INSTALL_LIBS=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Help function
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "OPTIONS:"
    echo "  --help        Show this help message"
    echo "  --clean       Clean build directory before building"
    echo "  --debug       Build in debug mode (default)"
    echo "  --release     Build in release mode"
    echo "  --with-tests  Build with Unity tests"
    echo "  --run-tests   Build and run Unity tests"
    echo "  --install     Install libraries and headers"
    echo ""
    echo "Examples:"
    echo "  $0                    # Basic build"
    echo "  $0 --clean --release # Clean release build"
    echo "  $0 --with-tests      # Build with tests"
    echo "  $0 --run-tests       # Build and run tests"
    echo "  $0 --install         # Build and install"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --help)
            show_help
            exit 0
            ;;
        --clean)
            CLEAN_BUILD=true
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
        --with-tests)
            BUILD_TESTS="ON"
            shift
            ;;
        --run-tests)
            BUILD_TESTS="ON"
            RUN_TESTS=true
            shift
            ;;
        --install)
            INSTALL_LIBS=true
            shift
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

echo -e "${GREEN}Building ZOO Thread Pool Module${NC}"
echo -e "${BLUE}Build type: $BUILD_TYPE${NC}"
echo -e "${BLUE}Tests: $BUILD_TESTS${NC}"
echo -e "${BLUE}Install: $INSTALL_LIBS${NC}"

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ] || [ ! -d "$BUILD_DIR" ]; then
    if [ -d "$BUILD_DIR" ]; then
        echo -e "${YELLOW}Cleaning old build directory...${NC}"
        rm -rf "$BUILD_DIR"
    fi
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${YELLOW}Configuring build...${NC}"
cmake "$SOURCE_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DZOO_THREAD_POOL_BUILD_TESTS="$BUILD_TESTS" \
    -DZOO_ENABLE_TESTS="$BUILD_TESTS"

# Build
echo -e "${YELLOW}Building...${NC}"
make -j$(nproc)

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo -e "${YELLOW}Running Unity tests...${NC}"
    cd tests
    
    # Run individual test executables
    for test_exe in test_*_unity; do
        if [ -x "$test_exe" ]; then
            echo -e "${BLUE}Running $test_exe...${NC}"
            ./"$test_exe"
        fi
    done
    
    cd ..
fi

# Install if requested
if [ "$INSTALL_LIBS" = true ]; then
    echo -e "${YELLOW}Installing libraries and headers...${NC}"
    make install
    echo -e "${GREEN}Installation completed to: $SOURCE_DIR/install${NC}"
fi

echo -e "${GREEN}Build completed successfully!${NC}"

if [ "$BUILD_TESTS" = "ON" ]; then
    echo -e "${GREEN}Unity test executables available in: $BUILD_DIR/tests/${NC}"
fi

if [ "$INSTALL_LIBS" = true ]; then
    echo -e "${GREEN}Libraries and headers installed to: $SOURCE_DIR/install${NC}"
    echo -e "${BLUE}Use pkg-config --cflags --libs zoo-thread-pool to get compiler flags${NC}"
fi
