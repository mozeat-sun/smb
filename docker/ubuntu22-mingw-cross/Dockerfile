FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    ca-certificates \
    bash \
    mingw-w64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/zoo

CMD ["bash"]
