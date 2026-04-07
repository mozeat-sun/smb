#!/bin/bash

set -euo pipefail

# Configuration
readonly BUILD_DIR="${BUILD_DIR:-build}"
readonly REPORT_DIR="${REPORT_DIR:-reports}"
readonly TIMESTAMP="${TIMESTAMP:-$(date +"%Y%m%d_%H%M%S")}"
readonly TEST_TIMEOUT="${TEST_TIMEOUT:-120}"

# Color definitions
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

log_info() {
    echo -e "${BLUE}[UNIT]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[UNIT]${NC} $*"
}

log_error() {
    echo -e "${RED}[UNIT]${NC} $*"
}

# Find the unit test executable
find_unit_test_executable() {
    local -a search_paths=(
        "${BUILD_DIR}/bin/zoo_smb_unit_tests"
        "${BUILD_DIR}/tests/zoo_smb_unit_tests"
        "${BUILD_DIR}/zoo_smb_unit_tests"
    )
    
    for path in "${search_paths[@]}"; do
        if [ -f "$path" ] && [ -x "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    
    return 1
}

# Run unit tests
run_unit_tests() {
    log_info "Starting unit tests..."
    
    local executable
    if ! executable=$(find_unit_test_executable); then
        log_error "Unit test executable not found"
        return 1
    fi
    
    log_info "Found executable: $executable"
    
    # Create report directory
    mkdir -p "${REPORT_DIR}"
    
    # Prepare output files
    local xml_output="${REPORT_DIR}/unit_test_results_${TIMESTAMP}.xml"
    local log_output="${REPORT_DIR}/unit_test_log_${TIMESTAMP}.txt"
    
    # Run tests
    log_info "Running unit tests (timeout: ${TEST_TIMEOUT}s)..."
    
    local start_time=$(date +%s)
    
    if timeout "$TEST_TIMEOUT" "$executable" \
        --gtest_output="xml:$xml_output" \
        --gtest_print_time=1 \
        --gtest_print_utf8=1 \
        --gtest_brief=1 \
        2>&1 | tee "$log_output"; then
        
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        log_success "Unit tests completed successfully in ${duration}s"
        
        # Show test statistics
        if [ -f "$xml_output" ]; then
            local total_tests=$(grep -c 'testcase' "$xml_output" || echo "0")
            local failed_tests=$(grep -c 'failure' "$xml_output" || echo "0")
            log_info "Test statistics: $total_tests total, $failed_tests failed"
        fi
        
        return 0
    else
        local exit_code=$?
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        if [ $exit_code -eq 124 ]; then
            log_error "Unit tests timed out after ${TEST_TIMEOUT}s"
        else
            log_error "Unit tests failed with exit code $exit_code after ${duration}s"
        fi
        
        return $exit_code
    fi
}

# Main function
main() {
    log_info "=== Unit Tests ==="
    
    if run_unit_tests; then
        log_success "Unit tests passed"
        exit 0
    else
        log_error "Unit tests failed"
        exit 1
    fi
}

main "$@"