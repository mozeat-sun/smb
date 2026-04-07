#!/bin/bash

# Script to build the ZOO SMB library
# Author: weiwang.sun
# Date: 2025-05-14

set -e

# Define colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Print with timestamp
log() {
    echo -e "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# --- Parse arguments ---
BUILD_TYPE="Debug"
CMAKE_EXTRA_FLAGS="-DZOO_SMB_AUTO_INIT=ON"

if [[ "$1" == "Test" || "$1" == "T" ]]; then
    BUILD_TYPE="Debug"
    CMAKE_EXTRA_FLAGS=""
    log "${GREEN}Test build: ZOO_SMB_AUTO_INIT enabled${NC}"
fi

# Only remove build directory if it does NOT contain googletest sources, and only for Release build
if [[ "$BUILD_TYPE" == "Release" ]]; then
    if [ -d "build" ]; then
        rm -rf build
    fi
fi

# Create and enter build directory
log "${GREEN}Creating build directory...${NC}"
mkdir -p build
cd build

# Generate build files with CMake
log "${GREEN}Generating build files...${NC}"
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${CMAKE_EXTRA_FLAGS} ..

# Build the project
log "${GREEN}Building project...${NC}"
cmake --build . -j$(nproc)

# Check build status
if [ $? -eq 0 ]; then
    log "${GREEN}Build completed successfully!${NC}"
else
    log "${RED}Build failed!${NC}"
    exit 1
fi


# Return to original directory
cd ..