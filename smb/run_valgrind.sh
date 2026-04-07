#!/bin/bash

set -euo pipefail  # Enable strict error handling

# Configuration variables
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_DIR="${BUILD_DIR:-build}"
readonly REPORT_DIR="${REPORT_DIR:-reports}"
readonly TIMESTAMP="${TIMESTAMP:-$(date +"%Y%m%d_%H%M%S")}"
readonly VALGRIND_TIMEOUT="${VALGRIND_TIMEOUT:-300}"  # 5 minutes timeout

# Color definitions
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

# Check dependencies
check_dependencies() {
    local missing_deps=()
    
    if ! command -v valgrind &> /dev/null; then
        missing_deps+=("valgrind")
    fi
    
    if ! command -v timeout &> /dev/null; then
        missing_deps+=("timeout")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_error "Missing dependencies: ${missing_deps[*]}"
        log_info "Install with: sudo apt-get install ${missing_deps[*]}"
        return 1
    fi
    
    return 0
}

# Find test executables
find_test_executables() {
    local -a test_paths=(
        "${BUILD_DIR}/bin/zoo_smb_unit_tests"
    )
    
    local -a found_executables=()
    
    for path in "${test_paths[@]}"; do
        if [ -f "$path" ] && [ -x "$path" ]; then
            found_executables+=("$path")
        fi
    done
    
    if [ ${#found_executables[@]} -eq 0 ]; then
        log_error "No test executables found in the following paths:"
        printf '  %s\n' "${test_paths[@]}"
        log_info "Build directory contents:"
        ls -la "${BUILD_DIR}/" 2>/dev/null || log_warning "Build directory not accessible"
        return 1
    fi
    
    printf '%s\n' "${found_executables[@]}"
    return 0
}

# Validate executable
validate_executable() {
    local executable="$1"
    
    if [ ! -f "$executable" ]; then
        log_error "Test executable not found: $executable"
        return 1
    fi
    
    if [ ! -x "$executable" ]; then
        log_error "Test executable not executable: $executable"
        log_info "File permissions: $(stat -c%A "$executable" 2>/dev/null || echo "unknown")"
        return 1
    fi
    
    # Test if executable can start normally
    if ! timeout 10 "$executable" --help &>/dev/null; then
        log_warning "Executable may have issues (help command failed)"
    fi
    
    return 0
}

# Run Valgrind analysis
run_valgrind_analysis() {
    local executable="$1"
    local report_file="$2"
    local test_filter="${3:-*}"
    local extra_args=("${@:4}")
    
    log_info "Starting Valgrind analysis"
    log_info "  Executable: $executable"
    log_info "  Report file: $report_file"
    log_info "  Test filter: $test_filter"
    log_info "  Timeout: ${VALGRIND_TIMEOUT}s"
    
    # Validate executable
    if ! validate_executable "$executable"; then
        return 1
    fi
    
    # Create report directory
    mkdir -p "$(dirname "$report_file")"
    
    # Prepare Valgrind arguments
    local -a valgrind_args=(
        --tool=memcheck
        --leak-check=full
        --show-leak-kinds=all
        --track-origins=yes
        --verbose
        --log-file="$report_file"
        --error-exitcode=1
        --child-silent-after-fork=yes
        --trace-children=no
        --gen-suppressions=all
    )
    # Add suppressions file if it exists
    local suppressions_file="/usr/share/gdb/auto-load/usr/lib/x86_64-linux-gnu/libthread_db-1.0.so.supp"
    if [ -f "$suppressions_file" ]; then
        valgrind_args+=(--suppressions="$suppressions_file")
    fi
    
    # Prepare test arguments
    local -a test_args=(
        --gtest_filter="$test_filter"
        --gtest_break_on_failure=false
        --gtest_catch_exceptions=true
        --gtest_print_time=false
        --gtest_print_utf8=false
    )
    
    # Add extra arguments if any
    if [ ${#extra_args[@]} -gt 0 ]; then
        test_args+=("${extra_args[@]}")
    fi
    
    log_info "Running Valgrind (this may take a while)..."
    
    # Run Valgrind with timeout
    local valgrind_exit_code=0
    if ! timeout "${VALGRIND_TIMEOUT}" valgrind "${valgrind_args[@]}" \
         "$executable" "${test_args[@]}" 2>&1; then
        valgrind_exit_code=$?
    fi
    
    # Handle timeout
    if [ $valgrind_exit_code -eq 124 ]; then
        log_error "Valgrind analysis timed out after ${VALGRIND_TIMEOUT}s"
        return 3
    fi
    
    # Check if report was generated
    if [ ! -f "$report_file" ]; then
        log_error "Valgrind report not generated: $report_file"
        log_info "Valgrind may have failed to run or encountered an error"
        return 1
    fi
    
    local report_size
    report_size=$(wc -l < "$report_file")
    log_success "Valgrind report generated: $report_file ($report_size lines)"
    
    # Analyze report
    analyze_valgrind_report "$report_file" "$valgrind_exit_code"
    return $?
}

# Analyze Valgrind report
analyze_valgrind_report() {
    local report_file="$1"
    local valgrind_exit_code="$2"
    
    log_info "Analyzing Valgrind report..."
    
    # Check error summary
    if grep -q "ERROR SUMMARY" "$report_file"; then
        local error_summary
        error_summary=$(grep "ERROR SUMMARY" "$report_file" | tail -1)
        log_info "Error Summary: $error_summary"
        
        # Extract error count
        local error_count
        error_count=$(echo "$error_summary" | grep -o '[0-9]\+' | head -1)
        
        if [ "$error_count" -gt 0 ]; then
            log_warning "Valgrind detected $error_count error(s)"
            
            # Show specific errors
            if grep -q "Invalid read\|Invalid write\|Invalid free" "$report_file"; then
                log_warning "Memory access violations detected:"
                grep -A 3 -B 1 "Invalid read\|Invalid write\|Invalid free" "$report_file" | head -20
            fi
            
            return 2
        fi
    fi
    
    # Check memory leaks
    if grep -q "LEAK SUMMARY" "$report_file"; then
        log_info "Memory leak summary:"
        grep -A 10 "LEAK SUMMARY" "$report_file" | head -15
        
        # Check for definite memory leaks
        if grep -q "definitely lost.*[1-9]" "$report_file"; then
            log_warning "Memory leaks detected"
            return 2
        elif grep -q "possibly lost.*[1-9]" "$report_file"; then
            log_warning "Possible memory leaks detected"
            return 2
        fi
    fi
    
    # Check heap summary
    if grep -q "HEAP SUMMARY" "$report_file"; then
        log_info "Heap summary:"
        grep -A 5 "HEAP SUMMARY" "$report_file"
    fi
    
    # Overall evaluation
    if [ "$valgrind_exit_code" -eq 0 ]; then
        log_success "No memory issues detected"
        return 0
    else
        log_warning "Valgrind detected issues (exit code: $valgrind_exit_code)"
        return 2
    fi
}

# Generate Valgrind summary report
generate_summary_report() {
    local -a report_files=("$@")
    local summary_file="${REPORT_DIR}/valgrind_summary_${TIMESTAMP}.txt"
    
    log_info "Generating summary report: $summary_file"
    
    {
        echo "Valgrind Analysis Summary"
        echo "========================"
        echo "Generated: $(date)"
        echo "Timestamp: $TIMESTAMP"
        echo ""
        
        for report_file in "${report_files[@]}"; do
            if [ -f "$report_file" ]; then
                echo "Report: $(basename "$report_file")"
                echo "----------------------------------------"
                
                # Extract key information
                if grep -q "ERROR SUMMARY" "$report_file"; then
                    grep "ERROR SUMMARY" "$report_file"
                fi
                
                if grep -q "LEAK SUMMARY" "$report_file"; then
                    grep -A 5 "LEAK SUMMARY" "$report_file"
                fi
                
                echo ""
            fi
        done
        
        echo "System Information:"
        echo "  OS: $(uname -s -r)"
        echo "  Memory: $(free -h | grep '^Mem:' | awk '{print $2}')"
        echo "  Valgrind Version: $(valgrind --version)"
        echo ""
        
    } > "$summary_file"
    
    log_success "Summary report generated: $summary_file"
}

# Cleanup function
cleanup() {
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        log_error "Script failed with exit code: $exit_code"
    fi
    
    # Clean up temporary files (if any)
    # rm -f /tmp/valgrind_temp_* 2>/dev/null || true
    
    exit $exit_code
}

# Main function
main() {
    local -a test_filters=("*InitializationTest*" "*CreateAndDestroyTest*" "*BasicFunctionTest*")
    local -a generated_reports=()
    
    log_info "Starting Valgrind analysis suite"
    log_info "Build directory: $BUILD_DIR"
    log_info "Report directory: $REPORT_DIR"
    
    # Set cleanup trap
    trap cleanup EXIT
    
    # Check dependencies
    if ! check_dependencies; then
        return 1
    fi
    
    # Create report directory
    mkdir -p "$REPORT_DIR"
    
    # Find test executables
    local -a executables
    if ! readarray -t executables < <(find_test_executables); then
        log_error "No test executables found"
        return 1
    fi
    
    log_success "Found ${#executables[@]} test executable(s)"
    
    # Run analysis for each executable
    for executable in "${executables[@]}"; do
        local base_name
        base_name=$(basename "$executable")
        
        log_info "Processing: $base_name"
        
        # Run analysis for different test filters
        for filter in "${test_filters[@]}"; do
            local report_file="${REPORT_DIR}/valgrind_${base_name}_${filter//\*/all}_${TIMESTAMP}.txt"
            
            if run_valgrind_analysis "$executable" "$report_file" "$filter"; then
                generated_reports+=("$report_file")
                log_success "Analysis completed for $base_name with filter $filter"
            else
                local exit_code=$?
                case $exit_code in
                    1) log_error "Analysis failed for $base_name with filter $filter" ;;
                    2) log_warning "Analysis completed with issues for $base_name with filter $filter" ;;
                    3) log_error "Analysis timed out for $base_name with filter $filter" ;;
                esac
                
                generated_reports+=("$report_file")
            fi
        done
        
        # Only process the first executable (can be adjusted as needed)
        # break
    done
    
    # Generate summary report
    generate_summary_report "${generated_reports[@]}"
    
    log_success "Valgrind analysis suite completed"
}

main "$@"