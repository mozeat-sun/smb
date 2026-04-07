#!/bin/bash

#******************************************************************************
# Copyright (C) 2025, ZOO Ltd.
# All rights reserved.
# Product: ZOO Timer Module
# File name: run_valgrind.sh
# Description: Valgrind memory testing script for ZOO timer module
# History recorder:
# Version   date           author            context
# 1.0       2025-08-01     AI Assistant      Created Valgrind testing script
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
echo "                     ZOO Timer Module Valgrind Testing"
echo "================================================================================"

# Check if Valgrind is available
if ! command -v valgrind >/dev/null 2>&1; then
    print_error "Valgrind is not installed. Please install valgrind and try again."
    exit 1
fi

# Run tests with Valgrind
"${PROJECT_DIR}/build.sh" --clean --valgrind

print_success "Valgrind memory testing completed successfully!"
