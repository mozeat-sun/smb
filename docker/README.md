# Docker Build Environments

This directory provides preconfigured Docker images for building ZOO on different platforms/toolchains.

## Available images

- ubuntu22-gcc: Ubuntu 22.04 + GCC + CMake + Ninja
- ubuntu24-clang: Ubuntu 24.04 + Clang/LLVM + CMake + Ninja
- debian12-gcc: Debian 12 + GCC + CMake + Ninja
- fedora40-gcc: Fedora 40 + GCC + CMake + Ninja
- alpine320-gcc: Alpine 3.20 (musl) + GCC + CMake + Ninja
- ubuntu22-aarch64-cross: Ubuntu 22.04 + aarch64-linux-gnu cross toolchain
- ubuntu22-mingw-cross: Ubuntu 22.04 + MinGW-w64 cross toolchain

## Build image

Run from repository root:

```bash
docker build -t zoo-build:ubuntu22-gcc -f docker/ubuntu22-gcc/Dockerfile .
```

## Start container for native build

```bash
docker run --rm -it \
  -v "$(pwd)":/workspace/zoo \
  -w /workspace/zoo \
  zoo-build:ubuntu22-gcc \
  bash
```

Inside container:

```bash
cmake -S . -B build -G Ninja
cmake --build build -j
```

## Clang build example

```bash
docker build -t zoo-build:ubuntu24-clang -f docker/ubuntu24-clang/Dockerfile .
docker run --rm -it -v "$(pwd)":/workspace/zoo -w /workspace/zoo zoo-build:ubuntu24-clang bash
cmake -S . -B build-clang -G Ninja
cmake --build build-clang -j
```

## AArch64 cross build example

```bash
docker build -t zoo-build:arm64-cross -f docker/ubuntu22-aarch64-cross/Dockerfile .
docker run --rm -it -v "$(pwd)":/workspace/zoo -w /workspace/zoo zoo-build:arm64-cross bash
cmake -S . -B build-arm64 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake
cmake --build build-arm64 -j
```

## MinGW-w64 cross build example

```bash
docker build -t zoo-build:mingw-cross -f docker/ubuntu22-mingw-cross/Dockerfile .
docker run --rm -it -v "$(pwd)":/workspace/zoo -w /workspace/zoo zoo-build:mingw-cross bash
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake
cmake --build build-mingw -j
```
