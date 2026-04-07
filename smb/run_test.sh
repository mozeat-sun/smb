#!/bin/bash

set -euo pipefail

# Configuration variables
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_DIR="${BUILD_DIR:-build}"
readonly REPORT_DIR="${REPORT_DIR:-reports}"
readonly TIMESTAMP="${TIMESTAMP:-$(date +"%Y%m%d_%H%M%S")}"
readonly PARALLEL_TESTS="${PARALLEL_TESTS:-false}"

# Color definitions
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly CYAN='\033[0;36m'
readonly BOLD='\033[1m'
readonly NC='\033[0m'

# Test script mapping
readonly -A TEST_SCRIPTS=(
    ["unit"]="run_unit_tests.sh"
    ["integration"]="run_integration_tests.sh"
    ["performance"]="run_performance_tests.sh"
    ["coverage"]="run_coverage.sh"
    ["valgrind"]="run_valgrind.sh"
)

# Test result tracking
declare -A TEST_RESULTS=()
declare -A TEST_TIMES=()
declare -A TEST_PIDS=()

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $(date '+%H:%M:%S') - $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $(date '+%H:%M:%S') - $*"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $(date '+%H:%M:%S') - $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $(date '+%H:%M:%S') - $*"
}

log_section() {
    echo -e "\n${CYAN}${BOLD}=========================================${NC}"
    echo -e "${CYAN}${BOLD}$*${NC}"
    echo -e "${CYAN}${BOLD}=========================================${NC}"
}

# Show usage help
show_help() {
    cat << EOF
Usage: $0 [OPTIONS] [TESTS...]

Run SMB Transport Library test suite.

OPTIONS:
    -h, --help          Show this help message
    -p, --parallel      Run tests in parallel (where possible)
    -s, --sequential    Run tests sequentially (default)
    -q, --quiet         Suppress verbose output
    -v, --verbose       Enable verbose output
    --skip-optional     Skip optional tests (coverage, valgrind)
    --build-dir DIR     Specify build directory (default: build)
    --report-dir DIR    Specify report directory (default: reports)

TESTS:
    unit               Run unit tests only
    integration        Run integration tests only
    performance        Run performance tests only
    coverage           Run coverage analysis only
    valgrind           Run valgrind analysis only

Examples:
    $0                          # Run all tests
    $0 unit integration         # Run only unit and integration tests
    $0 --parallel               # Run tests in parallel
    $0 --skip-optional          # Skip coverage and valgrind
    BUILD_DIR=mybuild $0        # Use custom build directory

EOF
}

# Parse command line arguments
parse_arguments() {
    local tests_to_run=()
    local skip_optional=false
    local quiet=false
    local verbose=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -p|--parallel)
                PARALLEL_TESTS=true
                shift
                ;;
            -s|--sequential)
                PARALLEL_TESTS=false
                shift
                ;;
            -q|--quiet)
                quiet=true
                shift
                ;;
            -v|--verbose)
                verbose=true
                shift
                ;;
            --skip-optional)
                skip_optional=true
                shift
                ;;
            --build-dir)
                BUILD_DIR="$2"
                shift 2
                ;;
            --report-dir)
                REPORT_DIR="$2"
                shift 2
                ;;
            unit|integration|performance|coverage|valgrind)
                tests_to_run+=("$1")
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Set default test list if none specified
    if [ ${#tests_to_run[@]} -eq 0 ]; then
        tests_to_run=("unit" "integration" "performance")
        if [ "$skip_optional" = false ]; then
            tests_to_run+=("coverage" "valgrind")
        fi
    fi
    
    # Export variables
    export BUILD_DIR REPORT_DIR TIMESTAMP
    
    # Set log level
    if [ "$quiet" = true ]; then
        exec 2>/dev/null
    fi
    
    printf '%s\n' "${tests_to_run[@]}"
}

# Check dependencies and environment
check_environment() {
    log_info "Checking environment..."
    
    # Check build directory
    if [ ! -d "$BUILD_DIR" ]; then
        log_error "Build directory not found: $BUILD_DIR"
        log_info "Please build the project first"
        return 1
    fi
    
    # Check test scripts
    local missing_scripts=()
    for test_name in "${!TEST_SCRIPTS[@]}"; do
        local script="${TEST_SCRIPTS[$test_name]}"
        if [ ! -f "$script" ]; then
            missing_scripts+=("$script")
        fi
    done
    
    if [ ${#missing_scripts[@]} -gt 0 ]; then
        log_warning "Missing test scripts: ${missing_scripts[*]}"
        log_info "Some tests may be skipped"
    fi
    
    # Create report directory
    mkdir -p "$REPORT_DIR"
    
    log_success "Environment check completed"
    return 0
}

# Run a single test script
run_single_test() {
    local test_name="$1"
    local script_name="${TEST_SCRIPTS[$test_name]}"
    
    log_info "Starting $test_name tests..."
    
    # Check if script exists
    if [ ! -f "$script_name" ]; then
        log_warning "Test script not found: $script_name"
        TEST_RESULTS["$test_name"]="SKIPPED"
        TEST_TIMES["$test_name"]=0
        return 0
    fi
    
    # Ensure script is executable
    chmod +x "$script_name"
    
    # Run test
    local start_time=$(date +%s)
    local result_code=0
    
    if "./$script_name" 2>&1 | sed "s/^/[$test_name] /"; then
        TEST_RESULTS["$test_name"]="PASSED"
        log_success "$test_name tests completed successfully"
    else
        result_code=$?
        TEST_RESULTS["$test_name"]="FAILED"
        log_error "$test_name tests failed with exit code $result_code"
    fi
    
    local end_time=$(date +%s)
    TEST_TIMES["$test_name"]=$((end_time - start_time))
    
    return $result_code
}

# Run a single test in background mode
run_test_background() {
    local test_name="$1"
    local script_name="${TEST_SCRIPTS[$test_name]}"
    
    if [ ! -f "$script_name" ]; then
        log_warning "Test script not found: $script_name"
        TEST_RESULTS["$test_name"]="SKIPPED"
        TEST_TIMES["$test_name"]=0
        return 0
    fi
    
    log_info "Starting $test_name tests in background..."
    
    # Ensure script is executable
    chmod +x "$script_name"
    
    # Run test and record PID
    local start_time=$(date +%s)
    
    (
        if "./$script_name" 2>&1 | sed "s/^/[$test_name] /"; then
            echo "PASSED" > "/tmp/test_result_${test_name}_$$"
        else
            echo "FAILED:$?" > "/tmp/test_result_${test_name}_$$"
        fi
        
        local end_time=$(date +%s)
        echo $((end_time - start_time)) > "/tmp/test_time_${test_name}_$$"
    ) &
    
    TEST_PIDS["$test_name"]=$!
    
    log_info "$test_name tests started with PID ${TEST_PIDS[$test_name]}"
}

# Wait for all background tests to complete
wait_for_background_tests() {
    log_info "Waiting for background tests to complete..."
    
    for test_name in "${!TEST_PIDS[@]}"; do
        local pid="${TEST_PIDS[$test_name]}"
        
        log_info "Waiting for $test_name (PID: $pid)..."
        
        if wait "$pid"; then
            log_success "$test_name background process completed"
        else
            log_error "$test_name background process failed"
        fi
        
        # Read result
        local result_file="/tmp/test_result_${test_name}_$$"
        local time_file="/tmp/test_time_${test_name}_$$"
        
        if [ -f "$result_file" ]; then
            local result=$(cat "$result_file")
            if [[ "$result" == "PASSED" ]]; then
                TEST_RESULTS["$test_name"]="PASSED"
            else
                TEST_RESULTS["$test_name"]="FAILED"
            fi
            rm -f "$result_file"
        else
            TEST_RESULTS["$test_name"]="UNKNOWN"
        fi
        
        if [ -f "$time_file" ]; then
            TEST_TIMES["$test_name"]=$(cat "$time_file")
            rm -f "$time_file"
        else
            TEST_TIMES["$test_name"]=0
        fi
    done
}

# Run the test suite
run_test_suite() {
    local tests_to_run=("$@")
    
    log_section "Running Test Suite"
    log_info "Tests to run: ${tests_to_run[*]}"
    log_info "Parallel mode: $PARALLEL_TESTS"
    log_info "Build directory: $BUILD_DIR"
    log_info "Report directory: $REPORT_DIR"
    
    local start_time=$(date +%s)
    
    # Run tests according to mode
    if [ "$PARALLEL_TESTS" = true ]; then
        # Run non-conflicting tests in parallel
        local parallel_tests=()
        local sequential_tests=()
        
        for test_name in "${tests_to_run[@]}"; do
            case "$test_name" in
                "coverage"|"valgrind")
                    sequential_tests+=("$test_name")
                    ;;
                *)
                    parallel_tests+=("$test_name")
                    ;;
            esac
        done
        
        # Run main tests in parallel
        if [ ${#parallel_tests[@]} -gt 0 ]; then
            log_info "Starting parallel tests: ${parallel_tests[*]}"
            for test_name in "${parallel_tests[@]}"; do
                run_test_background "$test_name"
            done
            wait_for_background_tests
        fi
        
        # Run analysis tests sequentially
        if [ ${#sequential_tests[@]} -gt 0 ]; then
            log_info "Starting sequential tests: ${sequential_tests[*]}"
            for test_name in "${sequential_tests[@]}"; do
                run_single_test "$test_name"
            done
        fi
    else
        # Run all tests sequentially
        for test_name in "${tests_to_run[@]}"; do
            run_single_test "$test_name"
        done
    fi
    
    local end_time=$(date +%s)
    local total_time=$((end_time - start_time))
    
    log_info "Test suite completed in ${total_time}s"
}

# Generate a comprehensive report
generate_comprehensive_report() {
    log_section "Generating Comprehensive Report"
    
    local summary_file="${REPORT_DIR}/test_summary_${TIMESTAMP}.txt"
    local html_file="${REPORT_DIR}/test_summary_${TIMESTAMP}.html"
    
    # Calculate statistics
    local total_tests=0
    local passed_tests=0
    local failed_tests=0
    local skipped_tests=0
    local total_time=0
    
    for test_name in "${!TEST_RESULTS[@]}"; do
        total_tests=$((total_tests + 1))
        total_time=$((total_time + TEST_TIMES["$test_name"]))
        
        case "${TEST_RESULTS[$test_name]}" in
            "PASSED") passed_tests=$((passed_tests + 1)) ;;
            "FAILED") failed_tests=$((failed_tests + 1)) ;;
            "SKIPPED") skipped_tests=$((skipped_tests + 1)) ;;
        esac
    done
    
    # Generate text report
    {
        echo "SMB Transport Library Test Report"
        echo "================================="
        echo "Generated: $(date)"
        echo "Timestamp: $TIMESTAMP"
        echo "Total Duration: ${total_time}s"
        echo ""
        echo "Test Results Summary:"
        echo "  Total Tests: $total_tests"
        echo "  Passed: $passed_tests"
        echo "  Failed: $failed_tests"
        echo "  Skipped: $skipped_tests"
        if [ $total_tests -gt 0 ]; then
            echo "  Success Rate: $(( (passed_tests * 100) / total_tests ))%"
        fi
        echo ""
        echo "Individual Test Results:"
        
        for test_name in "${!TEST_RESULTS[@]}"; do
            local status="${TEST_RESULTS[$test_name]}"
            local time="${TEST_TIMES[$test_name]}"
            printf "  %-15s: %-8s (%3ds)\n" "$test_name" "$status" "$time"
        done
        
        echo ""
        echo "Configuration:"
        echo "  Build Directory: $BUILD_DIR"
        echo "  Report Directory: $REPORT_DIR"
        echo "  Parallel Mode: $PARALLEL_TESTS"
        echo ""
        echo "System Information:"
        echo "  OS: $(uname -s -r)"
        echo "  Architecture: $(uname -m)"
        echo "  CPU: $(nproc) cores"
        echo "  Memory: $(free -h | grep '^Mem:' | awk '{print $2}')"
        echo ""
        echo "Generated Files:"
        find "$REPORT_DIR" -name "*${TIMESTAMP}*" -type f | sort
        
    } > "$summary_file"
    
    # Generate simple HTML report
    generate_html_report "$html_file" "$total_tests" "$passed_tests" "$failed_tests" "$skipped_tests"
    
    log_success "Reports generated:"
    log_info "  Text: $summary_file"
    log_info "  HTML: $html_file"
}

# Generate HTML report
generate_html_report() {
    local html_file="$1"
    local total="$2"
    local passed="$3"
    local failed="$4"
    local skipped="$5"
    
    cat > "$html_file" << EOF
<!DOCTYPE html>
<html>
<head>
    <title>SMB Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .header { background: #f8f9fa; padding: 20px; border-radius: 8px; margin-bottom: 20px; }
        .summary { display: flex; gap: 20px; margin-bottom: 20px; }
        .metric { background: #e9ecef; padding: 15px; border-radius: 8px; text-align: center; }
        .passed { color: #28a745; }
        .failed { color: #dc3545; }
        .skipped { color: #ffc107; }
        table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }
        th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background: #f8f9fa; }
        .footer { background: #f8f9fa; padding: 15px; border-radius: 8px; font-size: 0.9em; }
    </style>
</head>
<body>
    <div class="header">
        <h1>SMB Transport Library Test Report</h1>
        <p><strong>Generated:</strong> $(date)</p>
        <p><strong>Timestamp:</strong> $TIMESTAMP</p>
    </div>
    
    <div class="summary">
        <div class="metric">
            <h3>Total Tests</h3>
            <div style="font-size: 2em; font-weight: bold;">$total</div>
        </div>
        <div class="metric">
            <h3>Passed</h3>
            <div style="font-size: 2em; font-weight: bold;" class="passed">$passed</div>
        </div>
        <div class="metric">
            <h3>Failed</h3>
            <div style="font-size: 2em; font-weight: bold;" class="failed">$failed</div>
        </div>
        <div class="metric">
            <h3>Skipped</h3>
            <div style="font-size: 2em; font-weight: bold;" class="skipped">$skipped</div>
        </div>
    </div>
    
    <table>
        <tr>
            <th>Test Suite</th>
            <th>Status</th>
            <th>Duration</th>
        </tr>
EOF

    for test_name in "${!TEST_RESULTS[@]}"; do
        local status="${TEST_RESULTS[$test_name]}"
        local time="${TEST_TIMES[$test_name]}"
        local status_class=""
        
        case "$status" in
            "PASSED") status_class="passed" ;;
            "FAILED") status_class="failed" ;;
            "SKIPPED") status_class="skipped" ;;
        esac
        
        cat >> "$html_file" << EOF
        <tr>
            <td>$test_name</td>
            <td class="$status_class">$status</td>
            <td>${time}s</td>
        </tr>
EOF
    done
    
    cat >> "$html_file" << EOF
    </table>
    
    <div class="footer">
        <p><strong>Build Directory:</strong> $BUILD_DIR</p>
        <p><strong>Report Directory:</strong> $REPORT_DIR</p>
        <p><strong>System:</strong> $(uname -s -r) $(uname -m)</p>
    </div>
</body>
</html>
EOF
}

# Display final results
display_final_results() {
    log_section "Final Test Results"
    
    echo ""
    echo "=== TEST SUMMARY ==="
    
    local overall_success=true
    
    for test_name in "${!TEST_RESULTS[@]}"; do
        local status="${TEST_RESULTS[$test_name]}"
        local time="${TEST_TIMES[$test_name]}"
        
        case "$status" in
            "PASSED")
                echo -e "${test_name^} Tests: ${GREEN}✓ PASSED${NC} (${time}s)"
                ;;
            "FAILED")
                echo -e "${test_name^} Tests: ${RED}✗ FAILED${NC} (${time}s)"
                overall_success=false
                ;;
            "SKIPPED")
                echo -e "${test_name^} Tests: ${YELLOW}- SKIPPED${NC} (${time}s)"
                ;;
        esac
    done
    
    echo ""
    if [ "$overall_success" = true ]; then
        echo -e "${GREEN}${BOLD}🎉 All tests completed successfully!${NC}"
    else
        echo -e "${RED}${BOLD}❌ Some tests failed. Check reports for details.${NC}"
    fi
    
    echo ""
    echo -e "${CYAN}Reports: ${REPORT_DIR}/${NC}"
    echo -e "${CYAN}Summary: ${REPORT_DIR}/test_summary_${TIMESTAMP}.txt${NC}"
    echo -e "${CYAN}HTML: ${REPORT_DIR}/test_summary_${TIMESTAMP}.html${NC}"
    
    return $([[ "$overall_success" == true ]] && echo 0 || echo 1)
}

# Cleanup function
cleanup() {
    local exit_code=$?
    
    # Remove temporary files
    rm -f /tmp/test_result_*_$$ /tmp/test_time_*_$$ 2>/dev/null || true
    
    # Terminate background processes
    for pid in "${TEST_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            log_warning "Terminating background process: $pid"
            kill "$pid" 2>/dev/null || true
        fi
    done
    
    exit $exit_code
}

# Main function
main() {
    # Set cleanup trap
    trap cleanup EXIT INT TERM
    
    # Show startup info
    log_section "SMB Transport Library Test Suite"
    log_info "Script: $0"
    log_info "Arguments: $*"
    
    # Parse command line arguments
    local tests_to_run
    readarray -t tests_to_run < <(parse_arguments "$@")
    
    # Check environment
    check_environment || exit 1
    
    # Run tests
    run_test_suite "${tests_to_run[@]}"
    
    # Generate report
    generate_comprehensive_report
    
    # Display results
    display_final_results
    
    return $?
}

# Only run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi