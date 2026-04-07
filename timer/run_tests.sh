#!/bin/bash

#******************************************************************************
# Copyright (C) 2025, ZOO Ltd.
# All rights reserved.
# Product: ZOO Timer Module
# File name: run_tests.sh
# Description: Test execution script for ZOO timer module
# History recorder:
# Version   date           author            context
# 1.0       2025-08-01     AI Assistant      Created test runner script
#******************************************************************************

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ️  $1${NC}"
}

echo "================================================================================"
echo "                        ZOO Timer Module Test Runner"
echo "================================================================================"

# Run tests with build script
"${PROJECT_DIR}/build.sh" --clean --tests

print_success "All tests completed successfully!"
