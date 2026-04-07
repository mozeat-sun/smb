#!/bin/bash

# ZOO SMB Library Installation Script
# Author: weiwang.sun
# Date: 2025-05-14
# Description: Installs the ZOO SMB library and its dependencies

# Exit on any error
set -e

# Define colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Print with timestamp
log() {
    echo -e "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Check if running with sudo
if [ "$EUID" -ne 0 ]; then 
    log "${RED}Please run as root or with sudo${NC}"
    exit 1
fi

# Check if build directory exists
if [ ! -d "build" ]; then
    log "${RED}Build directory not found. Please run build.sh first.${NC}"
    exit 1
fi

# Enter build directory
cd build

# Install the library
log "${GREEN}Installing ZOO SMB library...${NC}"
cmake --build . --target install

# Refresh shared library cache
log "${GREEN}Updating shared library cache...${NC}"
ldconfig

# Check installation status
if [ $? -eq 0 ]; then
    log "${GREEN}Installation completed successfully!${NC}"
else
    log "${RED}Installation failed!${NC}"
    exit 1
fi

# Return to original directory
cd ..