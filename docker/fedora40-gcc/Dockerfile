FROM fedora:40

RUN dnf -y install \
    gcc \
    gcc-c++ \
    make \
    cmake \
    ninja-build \
    pkgconf-pkg-config \
    git \
    ca-certificates \
    bash \
    && dnf clean all

WORKDIR /workspace/zoo

CMD ["bash"]
