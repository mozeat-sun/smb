#!/bin/bash

set -euo pipefail

# Configurations
readonly BUILD_DIR="${BUILD_DIR:-build}"
readonly REPORT_DIR="${REPORT_DIR:-reports}"
readonly TIMESTAMP="${TIMESTAMP:-$(date +"%Y%m%d_%H%M%S")}"

# Color definitions
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

log_info() {
    echo -e "${BLUE}[COVERAGE]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[COVERAGE]${NC} $*"
}

log_warning() {
    echo -e "${YELLOW}[COVERAGE]${NC} $*"
}

log_error() {
    echo -e "${RED}[COVERAGE]${NC} $*"
}

# Check for coverage tools
check_coverage_tools() {
    if ! command -v gcov &> /dev/null; then
        log_error "gcov not found"
        return 1
    fi
    
    if ! command -v lcov &> /dev/null; then
        log_warning "lcov not found, will generate basic coverage only"
        return 2
    fi
    
    return 0
}

# Check if coverage is enabled
check_coverage_enabled() {
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
        log_error "CMakeCache.txt not found"
        return 1
    fi
    
    if ! grep -q "CMAKE_BUILD_TYPE.*Coverage\|CMAKE_CXX_FLAGS.*--coverage" "$BUILD_DIR/CMakeCache.txt"; then
        log_warning "Coverage not enabled in build configuration"
        log_info "Rebuild with: cmake -DCMAKE_BUILD_TYPE=Coverage"
        return 1
    fi
    
    return 0
}

# Generate coverage report
generate_coverage_report() {
    log_info "Generating coverage report..."
    
    local coverage_dir="${REPORT_DIR}/coverage_${TIMESTAMP}"
    mkdir -p "$coverage_dir"
    
    cd "$BUILD_DIR"
    
    # Find and process gcov files
    log_info "Processing gcov files..."
    find . -name "*.gcno" -exec gcov {} \; > /dev/null 2>&1
    
    # If lcov is available, generate HTML report
    if command -v lcov &> /dev/null; then
        log_info "Generating HTML coverage report..."
        
        # Capture coverage info
        lcov --capture --directory . --output-file coverage.info
        
        # Filter out unnecessary files
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --remove coverage.info '*/tests/*' --output-file coverage.info
        lcov --remove coverage.info '*/build/*' --output-file coverage.info
        lcov --remove coverage.info '*/external/*' --output-file coverage.info
        
        # Generate HTML report
        genhtml coverage.info --output-directory "$coverage_dir"
        
        log_success "HTML coverage report generated: $coverage_dir/index.html"
        
        # Show coverage summary
        if [ -f "coverage.info" ]; then
            log_info "Coverage summary:"
            lcov --summary coverage.info
        fi
        
        return 0
    else
        log_warning "lcov not available, generating basic coverage data only"
        
        # Generate basic coverage summary
        {
            echo "Coverage Report"
            echo "==============="
            echo "Generated: $(date)"
            echo "Timestamp: $TIMESTAMP"
            echo ""
            echo "gcov files found:"
            find . -name "*.gcov" | wc -l
            echo ""
            echo "Note: Install lcov for HTML reports"
        } > "$coverage_dir/coverage_summary.txt"
        
        # Copy gcov files
        find . -name "*.gcov" -exec cp {} "$coverage_dir/" \;
        
        log_success "Basic coverage data generated: $coverage_dir/"
        return 0
    fi
}

# Main function
main() {
    log_info "=== Coverage Analysis ==="
    
    # Check for coverage tools
    check_coverage_tools
    local tools_result=$?
    
    if [ $tools_result -eq 1 ]; then
        log_error "Required coverage tools not found"
        exit 1
    fi
    
    # Check if coverage is enabled
    if ! check_coverage_enabled; then
        log_error "Coverage not enabled in build"
        exit 1
    fi
    
    # Create report directory
    mkdir -p "${REPORT_DIR}"
    
    # Generate coverage report
    if generate_coverage_report; then
        log_success "Coverage analysis completed"
        exit 0
    else
        log_error "Coverage analysis failed"
        exit 1
    fi
}