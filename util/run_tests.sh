#!/bin/bash

# Quick Unity test runner for ZOO Util module
# This script builds and runs Unity tests quickly

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}Building and running ZOO Util Unity tests...${NC}"

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Quick build with Unity tests
echo -e "${YELLOW}Configuring with Unity tests...${NC}"
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

echo -e "${YELLOW}Building Unity tests...${NC}"
make -j$(nproc) zoo_util_tests_unity test_timestamp_unity test_string_unity test_memory_unity test_math_unity

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Run all tests
echo -e "${GREEN}Running Unity tests...${NC}"

echo -e "${BLUE}Running main test suite...${NC}"
./tests/zoo_util_tests_unity

if [ $? -eq 0 ]; then
    echo -e "${GREEN}All main tests passed!${NC}"
else
    echo -e "${RED}Some main tests failed!${NC}"
    exit 1
fi

echo -e "${BLUE}Running individual test suites...${NC}"

# Run individual test suites for detailed results
echo -e "${YELLOW}Timestamp tests:${NC}"
./tests/test_timestamp_unity

echo -e "${YELLOW}String tests:${NC}"
./tests/test_string_unity

echo -e "${YELLOW}Memory tests:${NC}"
./tests/test_memory_unity

echo -e "${YELLOW}Math tests:${NC}"
./tests/test_math_unity

echo -e "${GREEN}All Unity tests completed successfully!${NC}"
