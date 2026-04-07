FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    clang \
    lld \
    cmake \
    ninja-build \
    pkg-config \
    git \
    ca-certificates \
    build-essential \
    bash \
    && rm -rf /var/lib/apt/lists/*

ENV CC=clang
ENV CXX=clang++

WORKDIR /workspace/zoo

CMD ["bash"]
