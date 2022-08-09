# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

FROM fedora:33
LABEL maintainer="Ettus Research"

RUN dnf install -y \
        boost-devel \
        ccache \
        clang \
        cmake \
        doxygen \
        dpdk \
        dpdk-devel \
        dpdk-tools \
        gcc \
        gcc-c++ \
        git \
        libusb1-devel \
        make \
        ncompress \
        ninja-build \
        python3-devel \
        python3-docutils \
        python3-mako \
        python3-numpy \
        python3-pip \
        python3-requests \
        redhat-rpm-config \
        rpm-build \
        rpm-devel \
        rsync \
        tar \
        xz \
        && \
    dnf clean all
