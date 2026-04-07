#!/bin/bash

#******************************************************************************
# Copyright (C) 2025, Basic Software Research Institute ltd
# All rights reserved.
# Product: ZOO
# Module: dispatcher
# Component id: ZOO_DISPATCHER
# File name: build.sh
# Description: Build script for ZOO dispatcher module
# History recorder:
# Version   date           author            context
# 1.0       2025-08-04     AI Assistant      created
#******************************************************************************

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Build configuration
BUILD_TYPE=${BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-build}
INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}
JOBS=${JOBS:-$(nproc)}

# Function to print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check dependencies
check_dependencies() {
    print_info "Checking build dependencies..."
    
    # Detect platform
    PLATFORM=$(uname -s)
    ARCH=$(uname -m)
    print_info "Detected platform: $PLATFORM ($ARCH)"
    
    # Check for required tools
    for tool in cmake make gcc; do
        if ! command -v $tool &> /dev/null; then
            print_error "$tool is not installed or not in PATH"
            exit 1
        fi
    done
    
    # Check for pthread library
    if ! ldconfig -p | grep -q libpthread; then
        print_error "pthread library not found"
        exit 1
    fi
    
    print_info "All dependencies satisfied"
}

# Function to clean build directory
clean_build() {
    if [ -d "$BUILD_DIR" ]; then
        print_info "Cleaning existing build directory..."
        rm -rf "$BUILD_DIR"
    fi
}

# Function to configure the build
configure_build() {
    print_info "Configuring build with CMake..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          ..
    
    cd ..
}

# Function to build the project
build_project() {
    print_info "Building ZOO dispatcher module..."
    
    cd "$BUILD_DIR"
    make -j"$JOBS"
    cd ..
    
    print_info "Build completed successfully"
}

# Function to run tests
run_tests() {
    print_info "Running unit tests..."
    
    cd "$BUILD_DIR"
    if [ -f "tests/zoo_dispatcher_tests" ]; then
        ./tests/zoo_dispatcher_tests
        print_info "All tests passed"
    else
        print_warn "No test executable found, skipping tests"
    fi
    cd ..
}

# Function to install the project
install_project() {
    print_info "Installing ZOO dispatcher module to $INSTALL_PREFIX..."
    
    cd "$BUILD_DIR"
    sudo make install
    cd ..
    
    print_info "Installation completed"
}

# Function to package the project
package_project() {
    print_info "Creating distribution package..."
    
    cd "$BUILD_DIR"
    cpack
    cd ..
    
    print_info "Package created successfully"
}

# Main build function
main() {
    print_info "Starting ZOO dispatcher build process..."
    print_info "Build type: $BUILD_TYPE"
    print_info "Build directory: $BUILD_DIR"
    print_info "Install prefix: $INSTALL_PREFIX"
    print_info "Jobs: $JOBS"
    
    check_dependencies
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            clean)
                clean_build
                shift
                ;;
            configure)
                configure_build
                shift
                ;;
            build)
                build_project
                shift
                ;;
            test)
                run_tests
                shift
                ;;
            install)
                install_project
                shift
                ;;
            package)
                package_project
                shift
                ;;
            all)
                clean_build
                configure_build
                build_project
                run_tests
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Usage: $0 [clean|configure|build|test|install|package|all]"
                echo "  clean     - Clean build directory"
                echo "  configure - Configure build with CMake"
                echo "  build     - Build the project"
                echo "  test      - Run unit tests"
                echo "  install   - Install the project"
                echo "  package   - Create distribution package"
                echo "  all       - Clean, configure, build and test"
                exit 1
                ;;
        esac
    done
    
    # If no arguments provided, do a full build
    if [ $# -eq 0 ]; then
        clean_build
        configure_build
        build_project
        run_tests
    fi
    
    print_info "ZOO dispatcher build process completed successfully!"
}

# Run main function with all arguments
main "$@"
