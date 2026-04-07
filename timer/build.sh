#!/bin/bash

#******************************************************************************
# Copyright (C) 2025, ZOO Ltd.
# All rights reserved.
# Product: ZOO
# Module: Timer
# Component ID: TIMER
# File name: build.sh
# Description: Comprehensive build script for ZOO timer module
# History recorder:
# Version   date           author            context
# 1.0       2025-08-01     AI Assistant      Created comprehensive build system
#******************************************************************************

set -e  # Exit on any error

# ==============================================================================
# Build Configuration
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="${SCRIPT_DIR}"
BUILD_DIR="${PROJECT_DIR}/build"
TEST_BUILD_DIR="${PROJECT_DIR}/tests/build"
INSTALL_PREFIX="${PROJECT_DIR}/install"

# Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
BUILD_TYPE="${BUILD_TYPE:-Debug}"

# Number of parallel jobs
JOBS="${JOBS:-$(nproc)}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ==============================================================================
# Utility Functions
# ==============================================================================

print_header() {
    echo -e "${BLUE}"
    echo "================================================================================"
    echo "                        ZOO Timer Module Build System"
    echo "================================================================================"
    echo -e "${NC}"
}

print_section() {
    echo -e "${YELLOW}==> $1${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# ==============================================================================
# Help Function
# ==============================================================================

show_help() {
    cat << EOF
ZOO Timer Module Build Script

Usage: $0 [OPTIONS] [TARGETS]

OPTIONS:
    -h, --help              Show this help message
    -c, --clean             Clean build directories before building
    -t, --tests             Build and run tests
    -r, --release           Build in Release mode (default: Debug)
    -i, --install           Install after building
    -v, --verbose           Verbose output
    --coverage              Enable code coverage (implies --tests)
    --valgrind              Run tests with Valgrind (implies --tests)
    --static-analysis       Run static analysis tools
    --format-check          Check code formatting
    --unity-download        Download Unity testing framework

TARGETS:
    all                     Build library and examples (default)
    library                 Build only the timer library
    examples                Build only examples
    tests                   Build and run tests
    docs                    Generate documentation
    package                 Create distribution package
    install                 Install the library
    clean                   Clean build artifacts

EXAMPLES:
    $0                      # Build library and examples in Debug mode
    $0 --release            # Build in Release mode
    $0 --tests              # Build and run tests
    $0 --clean --tests      # Clean build and run tests
    $0 --coverage           # Build with coverage and run tests
    $0 --static-analysis    # Run static analysis tools

ENVIRONMENT VARIABLES:
    BUILD_TYPE              Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
    JOBS                    Number of parallel jobs (default: nproc)
    CC                      C compiler to use
    CXX                     C++ compiler to use
    CMAKE_ARGS              Additional CMake arguments

EOF
}

# ==============================================================================
# Dependency Management
# ==============================================================================

check_dependencies() {
    print_section "Checking Dependencies"
    
    local missing_deps=()
    
    # Check for required tools
    if ! command -v cmake >/dev/null 2>&1; then
        missing_deps+=("cmake")
    fi
    
    if ! command -v make >/dev/null 2>&1; then
        missing_deps+=("make")
    fi
    
    if ! command -v gcc >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
        missing_deps+=("gcc or clang")
    fi
    
    # Check for optional tools
    if ! command -v valgrind >/dev/null 2>&1; then
        print_info "Valgrind not found - memory testing will be skipped"
    fi
    
    if ! command -v lcov >/dev/null 2>&1; then
        print_info "lcov not found - coverage reporting will be limited"
    fi
    
    if ! command -v cppcheck >/dev/null 2>&1; then
        print_info "cppcheck not found - static analysis will be skipped"
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing required dependencies: ${missing_deps[*]}"
        echo "Please install the missing dependencies and try again."
        exit 1
    fi
    
    print_success "All required dependencies found"
}

download_unity() {
    print_section "Downloading Unity Testing Framework"
    
    local unity_dir="${PROJECT_DIR}/../third_party/Unity"
    local unity_version="v2.6.2"
    local unity_url="https://github.com/ThrowTheSwitch/Unity/archive/${unity_version}.tar.gz"
    
    if [ -d "${unity_dir}" ]; then
        print_info "Unity already exists at ${unity_dir}"
        return 0
    fi
    
    mkdir -p "${PROJECT_DIR}/../third_party"
    
    cd "${PROJECT_DIR}/../third_party"
    
    if command -v wget >/dev/null 2>&1; then
        wget -O unity.tar.gz "${unity_url}"
    elif command -v curl >/dev/null 2>&1; then
        curl -L -o unity.tar.gz "${unity_url}"
    else
        print_error "Neither wget nor curl found. Cannot download Unity."
        exit 1
    fi
    
    tar -xzf unity.tar.gz
    mv "Unity-${unity_version#v}" Unity
    rm unity.tar.gz
    
    print_success "Unity testing framework downloaded"
}

# ==============================================================================
# Build Functions
# ==============================================================================

clean_build() {
    print_section "Cleaning Build Directories"
    
    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        print_success "Cleaned main build directory"
    fi
    
    if [ -d "${TEST_BUILD_DIR}" ]; then
        rm -rf "${TEST_BUILD_DIR}"
        print_success "Cleaned test build directory"
    fi
    
    if [ -d "${INSTALL_PREFIX}" ]; then
        rm -rf "${INSTALL_PREFIX}"
        print_success "Cleaned install directory"
    fi
    
    # Clean coverage files
    find "${PROJECT_DIR}" -name "*.gcda" -delete 2>/dev/null || true
    find "${PROJECT_DIR}" -name "*.gcno" -delete 2>/dev/null || true
    find "${PROJECT_DIR}" -name "*.gcov" -delete 2>/dev/null || true
    
    print_success "Clean completed"
}

configure_build() {
    print_section "Configuring Build"
    
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
        "-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )
    
    # Add custom CMake arguments
    if [ -n "${CMAKE_ARGS}" ]; then
        cmake_args+=(${CMAKE_ARGS})
    fi
    
    # Add coverage flags if requested
    if [ "${ENABLE_COVERAGE}" = "1" ]; then
        cmake_args+=(
            "-DCMAKE_C_FLAGS=--coverage"
            "-DCMAKE_CXX_FLAGS=--coverage"
        )
    fi
    
    print_info "CMake configuration: ${cmake_args[*]}"
    
    cmake "${cmake_args[@]}" "${PROJECT_DIR}"
    
    print_success "Build configured"
}

build_library() {
    print_section "Building Timer Library"
    
    if [ ! -d "${BUILD_DIR}" ]; then
        configure_build
    fi
    
    cd "${BUILD_DIR}"
    make -j"${JOBS}" zoo_timer
    
    print_success "Timer library built successfully"
}

build_examples() {
    print_section "Building Examples"
    
    if [ ! -d "${BUILD_DIR}" ]; then
        configure_build
    fi
    
    cd "${BUILD_DIR}"
    
    if [ -d "${PROJECT_DIR}/examples" ]; then
        make -j"${JOBS}" examples
        print_success "Examples built successfully"
    else
        print_info "No examples directory found, skipping"
    fi
}

build_tests() {
    print_section "Building Tests"
    
    # Ensure Unity is available
    if [ ! -d "${PROJECT_DIR}/../third_party/Unity" ]; then
        download_unity
    fi
    
    mkdir -p "${TEST_BUILD_DIR}"
    cd "${TEST_BUILD_DIR}"
    
    local cmake_args=(
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
        "-DCMAKE_C_FLAGS=-g -O0 --coverage"
    )
    
    cmake "${cmake_args[@]}" "${PROJECT_DIR}/tests"
    make -j"${JOBS}"
    
    print_success "Tests built successfully"
}

run_tests() {
    print_section "Running Unit Tests"
    
    if [ ! -f "${TEST_BUILD_DIR}/zoo_timer_tests" ]; then
        build_tests
    fi
    
    cd "${TEST_BUILD_DIR}"
    
    if [ "${RUN_VALGRIND}" = "1" ] && command -v valgrind >/dev/null 2>&1; then
        print_info "Running tests with Valgrind"
        valgrind --leak-check=full --error-exitcode=1 ./zoo_timer_tests
    else
        ./zoo_timer_tests
    fi
    
    local test_result=$?
    
    if [ ${test_result} -eq 0 ]; then
        print_success "All tests passed"
    else
        print_error "Some tests failed"
        exit ${test_result}
    fi
}

generate_coverage() {
    print_section "Generating Coverage Report"
    
    if [ ! -d "${TEST_BUILD_DIR}" ]; then
        print_error "Tests must be run first to generate coverage"
        exit 1
    fi
    
    cd "${TEST_BUILD_DIR}"
    
    if command -v lcov >/dev/null 2>&1; then
        # Generate coverage report with lcov
        make coverage
        print_success "Coverage report generated in ${TEST_BUILD_DIR}/coverage/html"
        print_info "Open ${TEST_BUILD_DIR}/coverage/html/index.html to view the report"
    else
        # Fallback to gcov
        print_info "Using gcov for coverage analysis"
        find . -name "*.gcda" -exec gcov {} \\;
        print_success "Coverage files generated"
    fi
}

static_analysis() {
    print_section "Running Static Analysis"
    
    if command -v cppcheck >/dev/null 2>&1; then
        print_info "Running cppcheck"
        cppcheck --enable=all --error-exitcode=1 \
                 --suppress=missingIncludeSystem \
                 --suppress=unusedFunction \
                 "${PROJECT_DIR}/src" "${PROJECT_DIR}/inc"
        print_success "cppcheck analysis completed"
    else
        print_info "cppcheck not available, skipping static analysis"
    fi
    
    # Check for compile_commands.json and run clang-tidy if available
    if [ -f "${BUILD_DIR}/compile_commands.json" ] && command -v clang-tidy >/dev/null 2>&1; then
        print_info "Running clang-tidy"
        find "${PROJECT_DIR}/src" -name "*.c" -exec clang-tidy {} -p "${BUILD_DIR}" \\;
        print_success "clang-tidy analysis completed"
    fi
}

install_library() {
    print_section "Installing Library"
    
    if [ ! -d "${BUILD_DIR}" ]; then
        print_error "Library must be built before installation"
        exit 1
    fi
    
    cd "${BUILD_DIR}"
    make install
    
    print_success "Library installed to ${INSTALL_PREFIX}"
}

create_package() {
    print_section "Creating Distribution Package"
    
    if [ ! -d "${BUILD_DIR}" ]; then
        configure_build
        build_library
    fi
    
    cd "${BUILD_DIR}"
    make package
    
    print_success "Distribution package created"
}

# ==============================================================================
# Main Execution
# ==============================================================================

main() {
    print_header
    
    # Parse command line arguments
    local clean_build=0
    local build_tests=0
    local enable_coverage=0
    local run_valgrind=0
    local run_static_analysis=0
    local format_check=0
    local install_after_build=0
    local verbose=0
    local download_unity_flag=0
    local targets=()
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                clean_build=1
                shift
                ;;
            -t|--tests)
                build_tests=1
                shift
                ;;
            -r|--release)
                BUILD_TYPE="Release"
                shift
                ;;
            -i|--install)
                install_after_build=1
                shift
                ;;
            -v|--verbose)
                verbose=1
                set -x
                shift
                ;;
            --coverage)
                enable_coverage=1
                build_tests=1
                ENABLE_COVERAGE=1
                export ENABLE_COVERAGE
                shift
                ;;
            --valgrind)
                run_valgrind=1
                build_tests=1
                RUN_VALGRIND=1
                export RUN_VALGRIND
                shift
                ;;
            --static-analysis)
                run_static_analysis=1
                shift
                ;;
            --format-check)
                format_check=1
                shift
                ;;
            --unity-download)
                download_unity_flag=1
                shift
                ;;
            *)
                targets+=("$1")
                shift
                ;;
        esac
    done
    
    # Set default target if none specified
    if [ ${#targets[@]} -eq 0 ]; then
        targets=("all")
    fi
    
    # Check dependencies
    check_dependencies
    
    # Clean if requested
    if [ ${clean_build} -eq 1 ]; then
        clean_build
    fi
    
    # Download Unity if requested
    if [ ${download_unity_flag} -eq 1 ]; then
        download_unity
    fi
    
    # Process targets
    for target in "${targets[@]}"; do
        case ${target} in
            all)
                configure_build
                build_library
                build_examples
                ;;
            library)
                configure_build
                build_library
                ;;
            examples)
                configure_build
                build_examples
                ;;
            tests)
                build_tests
                run_tests
                ;;
            clean)
                clean_build
                ;;
            install)
                install_library
                ;;
            package)
                create_package
                ;;
            docs)
                print_info "Documentation generation not implemented yet"
                ;;
            *)
                print_error "Unknown target: ${target}"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Additional actions based on flags
    if [ ${build_tests} -eq 1 ]; then
        build_tests
        run_tests
        
        if [ ${enable_coverage} -eq 1 ]; then
            generate_coverage
        fi
    fi
    
    if [ ${run_static_analysis} -eq 1 ]; then
        static_analysis
    fi
    
    if [ ${install_after_build} -eq 1 ]; then
        install_library
    fi
    
    print_success "Build completed successfully!"
    print_info "Build type: ${BUILD_TYPE}"
    print_info "Build directory: ${BUILD_DIR}"
    
    if [ ${build_tests} -eq 1 ]; then
        print_info "Test results available in: ${TEST_BUILD_DIR}"
    fi
    
    if [ ${enable_coverage} -eq 1 ]; then
        print_info "Coverage report: ${TEST_BUILD_DIR}/coverage/html/index.html"
    fi
}

# Execute main function with all arguments
main "$@"
