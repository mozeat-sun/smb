#!/bin/bash

set -euo pipefail

# Configuration
readonly BUILD_DIR="${BUILD_DIR:-build}"
readonly REPORT_DIR="${REPORT_DIR:-reports}"
readonly TIMESTAMP="${TIMESTAMP:-$(date +"%Y%m%d_%H%M%S")}"
readonly TEST_TIMEOUT="${TEST_TIMEOUT:-600}"

# Color definitions
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

log_info() {
    echo -e "${BLUE}[PERFORMANCE]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[PERFORMANCE]${NC} $*"
}

log_error() {
    echo -e "${RED}[PERFORMANCE]${NC} $*"
}

# Find the performance test executable
find_performance_test_executable() {
    local -a search_paths=(
        "${BUILD_DIR}/bin/zoo_smb_performance_tests"
        "${BUILD_DIR}/tests/zoo_smb_performance_tests"
        "${BUILD_DIR}/zoo_smb_performance_tests"
    )
    
    for path in "${search_paths[@]}"; do
        if [ -f "$path" ] && [ -x "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    
    return 1
}

# Run performance tests
run_performance_tests() {
    log_info "Starting performance tests..."
    
    local executable
    if ! executable=$(find_performance_test_executable); then
        log_error "Performance test executable not found"
        return 1
    fi
    
    log_info "Found executable: $executable"
    
    # Create report directory
    mkdir -p "${REPORT_DIR}"
    
    # Prepare output files
    local xml_output="${REPORT_DIR}/performance_test_results_${TIMESTAMP}.xml"
    local log_output="${REPORT_DIR}/performance_test_log_${TIMESTAMP}.txt"
    local perf_output="${REPORT_DIR}/performance_metrics_${TIMESTAMP}.json"
    
    # Run tests
    log_info "Running performance tests (timeout: ${TEST_TIMEOUT}s)..."
    log_info "Note: Performance tests may take longer to complete"
    
    local start_time=$(date +%s)
    
    if timeout "$TEST_TIMEOUT" "$executable" \
        --gtest_output="xml:$xml_output" \
        --gtest_print_time=1 \
        --gtest_print_utf8=1 \
        --gtest_brief=1 \
        2>&1 | tee "$log_output"; then
        
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        log_success "Performance tests completed successfully in ${duration}s"
        
        # Show test statistics
        if [ -f "$xml_output" ]; then
            local total_tests=$(grep -c 'testcase' "$xml_output" || echo "0")
            local failed_tests=$(grep -c 'failure' "$xml_output" || echo "0")
            log_info "Test statistics: $total_tests total, $failed_tests failed"
        fi
        
        # Extract performance metrics
        extract_performance_metrics "$log_output" "$perf_output"
        
        return 0
    else
        local exit_code=$?
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        
        if [ $exit_code -eq 124 ]; then
            log_error "Performance tests timed out after ${TEST_TIMEOUT}s"
        else
            log_error "Performance tests failed with exit code $exit_code after ${duration}s"
        fi
        
        return $exit_code
    fi
}

# Extract performance metrics
extract_performance_metrics() {
    local log_file="$1"
    local output_file="$2"
    
    if [ ! -f "$log_file" ]; then
        return 1
    fi
    
    log_info "Extracting performance metrics..."
    
    # Extract key performance metrics
    {
        echo "{"
        echo "  \"timestamp\": \"$TIMESTAMP\","
        echo "  \"system_info\": {"
        echo "    \"cpu_cores\": $(nproc),"
        echo "    \"memory_gb\": \"$(free -h | grep '^Mem:' | awk '{print $2}')\","
        echo "    \"os\": \"$(uname -s -r)\""
        echo "  },"
        echo "  \"metrics\": {"
        
        # Find throughput information
        local throughput=$(grep -o "[0-9.]\+ MB/s" "$log_file" | tail -1 || echo "0")
        echo "    \"throughput_mbps\": \"$throughput\","
        
        # Find message rate
        local msg_rate=$(grep -o "[0-9.]\+ messages/sec" "$log_file" | tail -1 || echo "0")
        echo "    \"message_rate\": \"$msg_rate\","
        
        # Find latency information
        local latency=$(grep -o "[0-9.]\+ microseconds" "$log_file" | tail -1 || echo "0")
        echo "    \"average_latency_us\": \"$latency\""
        
        echo "  }"
        echo "}"
    } > "$output_file"
    
    log_info "Performance metrics saved to: $output_file"
}

# Main function
main() {
    log_info "=== Performance Tests ==="
    
    if run_performance_tests; then
        log_success "Performance tests passed"
        exit 0
    else
        log_error "Performance tests failed"
        exit 1
    fi
}

main "$@"