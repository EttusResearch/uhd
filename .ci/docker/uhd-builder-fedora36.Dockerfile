# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

FROM fedora:36
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
        && \
    dnf clean all
