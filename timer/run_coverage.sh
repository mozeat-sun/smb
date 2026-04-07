#!/bin/bash

#******************************************************************************
# Copyright (C) 2025, ZOO Ltd.
# All rights reserved.
# Product: ZOO Timer Module
# File name: run_coverage.sh
# Description: Coverage analysis script for ZOO timer module
# History recorder:
# Version   date           author            context
# 1.0       2025-08-01     AI Assistant      Created coverage analysis script
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
echo "                      ZOO Timer Module Coverage Analysis"
echo "================================================================================"

# Run tests with coverage
"${PROJECT_DIR}/build.sh" --clean --coverage

# Open coverage report if available
COVERAGE_HTML="${PROJECT_DIR}/tests/build/coverage/html/index.html"
if [ -f "${COVERAGE_HTML}" ]; then
    print_info "Coverage report generated: ${COVERAGE_HTML}"
    
    # Try to open in browser
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "${COVERAGE_HTML}" 2>/dev/null || true
    elif command -v open >/dev/null 2>&1; then
        open "${COVERAGE_HTML}" 2>/dev/null || true
    fi
fi

print_success "Coverage analysis completed!"
