# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

FROM fedora:42
LABEL maintainer="Ettus Research"

ARG PIP_INDEX_HOST
ARG PIP_INDEX_URL

RUN dnf install -y \
        boost-devel \
        ccache \
        clang \
        cmake \
        doxygen \
        dpdk \
        dpdk-devel \
        dpdk-tools \
        gcc-15.0.1-0.11.fc42 \
        gcc-c++-15.0.1-0.11.fc42 \
        git \
        libusb1-devel \
        make \
        ncompress \
        ninja-build \
        pybind11-devel \
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
    # Install GNURadio dependencies
        python3-sphinx \
        python3-lxml \
        SDL-devel \
        gsl-devel \
        qwt-qt5-devel \
        qt5-qtbase-devel \
        gmp-devel \
        fftw-devel \
        swig \
        gtk3-devel \
        pango-devel \
        PyQt5 \
        log4cpp-devel \
        zeromq-devel \
        cppzmq-devel \
        python3-ruamel-yaml \
        python3-click \
        python3-click-plugins \
        python3-zmq \
        python3-scipy \
        python3-cairo-devel \
        python3-gobject \
        spdlog-devel \
        && \
    dnf clean all

# Optionally use cached index.
RUN if [[ -n "$PIP_INDEX_URL" ]]; then \
        python3 -m pip config --global set global.index-url $PIP_INDEX_URL && \
        python3 -m pip config --global set global.trusted-host $PIP_INDEX_HOST; \
    fi

RUN python3 -m pip config list
RUN python3 -m pip install --upgrade pip
RUN pip install pygccxml pyyaml
