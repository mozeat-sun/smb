#!/bin/bash

# ZOO Memory Pool Build Script
# Cross-platform memory pool implementation build automation

set -e  # Exit on any error

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
SOURCE_DIR="$(pwd)"
CMAKE_BUILD_TYPE="Release"
BUILD_TESTS=ON  # Default to Unity tests enabled
VERBOSE=OFF
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
UNITY_TESTS=ON  # Enable Unity testing by default

# Print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Help function
show_help() {
    cat << EOF
ZOO Memory Pool Build Script with Unity/CMock Testing

Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help          Show this help message
    -d, --debug         Build in debug mode (default: release)
    -r, --release       Build in release mode (explicit)
    -t, --test          Enable tests (Unity/CMock - default: ON)
    --no-tests          Disable tests
    --unity             Enable Unity tests (default)
    --gtest             Enable GTest (legacy - not recommended)
    -c, --clean         Clean build directory before building
    -v, --verbose       Verbose build output
    -j, --jobs JOBS     Number of parallel jobs (default: $JOBS)
    --build-dir DIR     Build directory (default: $BUILD_DIR)
    --install           Install after successful build
    --cross-compile TARGET   Cross-compile for target (arm, arm64, x86)
    --run-tests         Run tests after building (Unity tests only)

UNITY/CMOCK TESTING:
    The build script now defaults to Unity C Testing Framework with CMock support.
    This replaces the previous GTest implementation for better C integration.
    
    Test Features:
    - Unity C Testing Framework v2.6.2
    - CMock mocking framework v2.6.1
    - Platform detection (X86_64, ARM, etc.)
    - Thread safety testing
    - Memory statistics validation
    - Comprehensive coverage (11 tests)

EXAMPLES:
    $0                      Build release with Unity tests
    $0 --debug --run-tests  Build debug and run tests
    $0 --clean --unity      Clean rebuild with Unity tests
    $0 --no-tests           Build without any tests
    $0 --cross-compile arm  Cross-compile for ARM

TEST EXECUTION:
    After building with tests enabled:
    ./build/bin/zoo_memory_pool_tests    # Run Unity tests directly
    ctest --output-on-failure           # Run via CTest
    make -C build quick_test             # Run via CMake target

EOF
}

# Check dependencies
check_dependencies() {
    print_info "Checking build dependencies..."
    
    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake not found. Please install CMake 3.14 or later."
        exit 1
    fi
    
    # Check CMake version
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    CMAKE_MAJOR=$(echo $CMAKE_VERSION | cut -d'.' -f1)
    CMAKE_MINOR=$(echo $CMAKE_VERSION | cut -d'.' -f2)
    
    if [ "$CMAKE_MAJOR" -lt 3 ] || ([ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -lt 14 ]); then
        print_error "CMake 3.14 or later required. Found: $CMAKE_VERSION"
        exit 1
    fi
    
    # Check for compiler
    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        print_error "No suitable C compiler found (gcc or clang required)"
        exit 1
    fi
    
    # Check Unity/CMock dependencies if tests enabled
    if [ "$BUILD_TESTS" = "ON" ] && [ "$UNITY_TESTS" = "ON" ]; then
        check_unity_dependencies
    fi
    
    print_success "Dependencies check passed"
}

# Check Unity testing framework dependencies
check_unity_dependencies() {
    print_info "Checking Unity/CMock testing framework..."
    
    # Get the absolute path to the zoo root directory
    ZOO_ROOT="$(cd "$(dirname "$SOURCE_DIR")" && pwd)"
    
    # Check for ZOO platform
    if [ ! -d "$ZOO_ROOT/platform/inc" ]; then
        print_error "ZOO platform not found at $ZOO_ROOT/platform/inc"
        print_error "Please ensure the platform module is available"
        exit 1
    fi
    
    # Check for ZOO util
    if [ ! -d "$ZOO_ROOT/util" ]; then
        print_error "ZOO util not found at $ZOO_ROOT/util"
        print_error "Please ensure the util module is available"
        exit 1
    fi
    
    # Check for Unity framework
    UNITY_PATH="$ZOO_ROOT/third_party/test/unity"
    if [ ! -d "$UNITY_PATH" ]; then
        print_error "Unity framework not found at $UNITY_PATH"
        print_error "Please ensure Unity testing framework is available"
        exit 1
    fi
    
    # Check for CMock framework
    CMOCK_PATH="$ZOO_ROOT/third_party/test/CMock/src"
    if [ ! -d "$CMOCK_PATH" ]; then
        print_warning "CMock framework not found at $CMOCK_PATH"
        print_warning "CMock features will not be available"
    else
        print_info "CMock framework found at: $CMOCK_PATH"
    fi
    
    print_info "Unity framework found at: $UNITY_PATH"
    print_success "Unity/CMock dependencies check passed"
}

# Platform detection
detect_platform() {
    print_info "Detecting platform..."
    
    OS=$(uname -s)
    ARCH=$(uname -m)
    
    case "$OS" in
        Linux*)
            PLATFORM="Linux"
            ;;
        Darwin*)
            PLATFORM="macOS"
            ;;
        CYGWIN*|MINGW32*|MSYS*|MINGW*)
            PLATFORM="Windows"
            ;;
        *)
            PLATFORM="Unknown"
            print_warning "Unknown platform: $OS"
            ;;
    esac
    
    print_info "Platform: $PLATFORM ($ARCH)"
}

# Clean build directory
clean_build() {
    if [ -d "$BUILD_DIR" ]; then
        print_info "Cleaning build directory: $BUILD_DIR"
        rm -rf "$BUILD_DIR"
    fi
}

# Configure build
configure_build() {
    print_info "Configuring build..."
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    CMAKE_ARGS=(
        "-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
        "-DBUILD_TESTING=$BUILD_TESTS"
        "-DZOO_MEMORY_POOL_BUILD_TESTS=$BUILD_TESTS"
    )
    
    # Add Unity-specific configurations
    if [ "$BUILD_TESTS" = "ON" ] && [ "$UNITY_TESTS" = "ON" ]; then
        CMAKE_ARGS+=("-DUNITY_TESTING=ON")
        print_info "Configuring with Unity/CMock testing framework"
    elif [ "$BUILD_TESTS" = "ON" ]; then
        print_warning "Non-Unity testing requested but not fully supported"
    else
        print_info "Tests disabled"
    fi
    
    # Add cross-compilation settings if specified
    if [ -n "$CROSS_COMPILE_TARGET" ]; then
        case "$CROSS_COMPILE_TARGET" in
            arm)
                CMAKE_ARGS+=("-DCMAKE_SYSTEM_NAME=Linux")
                CMAKE_ARGS+=("-DCMAKE_SYSTEM_PROCESSOR=arm")
                CMAKE_ARGS+=("-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc")
                ;;
            arm64)
                CMAKE_ARGS+=("-DCMAKE_SYSTEM_NAME=Linux")
                CMAKE_ARGS+=("-DCMAKE_SYSTEM_PROCESSOR=aarch64")
                CMAKE_ARGS+=("-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc")
                ;;
            x86)
                CMAKE_ARGS+=("-DCMAKE_C_FLAGS=-m32")
                ;;
            *)
                print_error "Unknown cross-compile target: $CROSS_COMPILE_TARGET"
                exit 1
                ;;
        esac
        print_info "Cross-compiling for: $CROSS_COMPILE_TARGET"
    fi
    
    # Run CMake
    cmake "${CMAKE_ARGS[@]}" "$SOURCE_DIR"
    
    cd "$SOURCE_DIR"
    print_success "Configuration completed"
}

# Build the project
build_project() {
    print_info "Building ZOO Memory Pool..."
    
    cd "$BUILD_DIR"
    
    BUILD_ARGS=(
        "--build" "."
        "--config" "$CMAKE_BUILD_TYPE"
        "--parallel" "$JOBS"
    )
    
    if [ "$VERBOSE" = "ON" ]; then
        BUILD_ARGS+=("--verbose")
    fi
    
    cmake "${BUILD_ARGS[@]}"
    
    cd "$SOURCE_DIR"
    print_success "Build completed successfully"
}

# Run tests
run_tests() {
    if [ "$BUILD_TESTS" = "ON" ]; then
        print_info "Running tests..."
        
        cd "$BUILD_DIR"
        
        if [ "$UNITY_TESTS" = "ON" ]; then
            # Run Unity tests directly
            if [ -f "bin/zoo_memory_pool_tests" ]; then
                print_info "Running Unity/CMock tests..."
                ./bin/zoo_memory_pool_tests
                print_success "Unity tests completed"
            else
                print_warning "Unity test executable not found at bin/zoo_memory_pool_tests"
            fi
            
            # Also run via CTest if available
            if command -v ctest &> /dev/null; then
                print_info "Running tests via CTest..."
                ctest --output-on-failure --parallel "$JOBS"
            fi
        else
            # Run generic tests via CTest
            ctest --output-on-failure --parallel "$JOBS"
        fi
        
        cd "$SOURCE_DIR"
        print_success "All tests completed"
    fi
}

# Install the project
install_project() {
    print_info "Installing ZOO Memory Pool..."
    
    cd "$BUILD_DIR"
    cmake --install .
    cd "$SOURCE_DIR"
    
    print_success "Installation completed"
}

# Show build summary
show_summary() {
    print_info "Build Summary:"
    echo "  Source Directory: $SOURCE_DIR"
    echo "  Build Directory:  $BUILD_DIR"
    echo "  Build Type:       $CMAKE_BUILD_TYPE"
    echo "  Tests Enabled:    $BUILD_TESTS"
    if [ "$BUILD_TESTS" = "ON" ]; then
        echo "  Test Framework:   Unity/CMock"
    fi
    echo "  Parallel Jobs:    $JOBS"
    if [ -n "$CROSS_COMPILE_TARGET" ]; then
        echo "  Cross Compile:    $CROSS_COMPILE_TARGET"
    fi
    echo ""
    echo "Build artifacts:"
    echo "  Libraries:        $BUILD_DIR/lib/"
    if [ "$BUILD_TESTS" = "ON" ]; then
        echo "  Test Executable:  $BUILD_DIR/bin/zoo_memory_pool_tests"
        echo ""
        echo "To run tests manually:"
        echo "  $BUILD_DIR/bin/zoo_memory_pool_tests"
        echo "  cd $BUILD_DIR && ctest --output-on-failure"
        echo "  make -C $BUILD_DIR quick_test"
    fi
}

# Main execution
main() {
    # Parse command line arguments
    CLEAN_BUILD=false
    INSTALL_AFTER_BUILD=false
    RUN_TESTS_AFTER_BUILD=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -d|--debug)
                CMAKE_BUILD_TYPE="Debug"
                shift
                ;;
            -r|--release)
                CMAKE_BUILD_TYPE="Release"
                shift
                ;;
            -t|--test)
                BUILD_TESTS=ON
                UNITY_TESTS=ON
                shift
                ;;
            --no-tests)
                BUILD_TESTS=OFF
                UNITY_TESTS=OFF
                shift
                ;;
            --unity)
                BUILD_TESTS=ON
                UNITY_TESTS=ON
                shift
                ;;
            --gtest)
                BUILD_TESTS=ON
                UNITY_TESTS=OFF
                print_warning "GTest support is legacy. Unity/CMock is recommended."
                shift
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=ON
                shift
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            --build-dir)
                BUILD_DIR="$2"
                shift 2
                ;;
            --install)
                INSTALL_AFTER_BUILD=true
                shift
                ;;
            --run-tests)
                RUN_TESTS_AFTER_BUILD=true
                shift
                ;;
            --cross-compile)
                CROSS_COMPILE_TARGET="$2"
                shift 2
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Execute build process
    print_info "Starting ZOO Memory Pool build process..."
    
    detect_platform
    check_dependencies
    
    if [ "$CLEAN_BUILD" = true ]; then
        clean_build
    fi
    
    configure_build
    show_summary
    build_project
    
    if [ "$RUN_TESTS_AFTER_BUILD" = true ]; then
        run_tests
    fi
    
    if [ "$INSTALL_AFTER_BUILD" = true ]; then
        install_project
    fi
    
    print_success "ZOO Memory Pool build process completed successfully!"
    
    # Show additional information for Unity tests
    if [ "$BUILD_TESTS" = "ON" ] && [ "$UNITY_TESTS" = "ON" ] && [ "$RUN_TESTS_AFTER_BUILD" = false ]; then
        echo ""
        print_info "Unity/CMock tests are ready. To run tests:"
        echo "  $BUILD_DIR/bin/zoo_memory_pool_tests"
        echo "  Or use: $0 --run-tests"
    fi
}

# Run main function with all arguments
main "$@"
