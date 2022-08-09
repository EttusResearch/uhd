# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

FROM ubuntu:20.04
LABEL maintainer="Ettus Research"

# This will make apt-get install without question
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get -y upgrade && \
    apt-get -y install -q \
        build-essential \
        ccache \
        clang \
        curl \
        git \
        sudo \
    # Install UHD dependencies
        abi-dumper \
        cmake \
        doxygen \
        dpdk \
        libboost-all-dev \
        libdpdk-dev \
        libgps-dev \
        libgps-dev \
        libudev-dev \
        libusb-1.0-0-dev \
        ncompress \
        ninja-build \
        python3-dev \
        python3-docutils \
        python3-mako \
        python3-numpy \
        python3-pip \
        python3-requests \
    # Install deb dependencies
        debootstrap \
        devscripts \
        pbuilder \
        debhelper \
        libncurses5-dev \
        python3-ruamel.yaml \
    # Install GNURadio dependencies
        python3-sphinx \
        python3-lxml \
        libsdl1.2-dev \
        libgsl-dev \
        libqwt-qt5-dev \
        libqt5opengl5-dev \
        libgmp3-dev \
        libfftw3-dev \
        swig \
        gir1.2-gtk-3.0 \
        libpango1.0-dev \
        python3-pyqt5 \
        liblog4cpp5-dev \
        libzmq3-dev \
        python3-ruamel.yaml \
        python3-click \
        python3-click-plugins \
        python3-zmq \
        python3-scipy \
        python3-gi-cairo \
        && \
    rm -rf /var/lib/apt/lists/*

# Required for running pbuilder to build debs in docker on Ubuntu 20.04
# because of this bug: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=968927
RUN wget https://launchpad.net/ubuntu/+archive/primary/+files/debootstrap_1.0.124_all.deb && \
    dpkg -i debootstrap_1.0.124_all.deb && \
    rm debootstrap_1.0.124_all.deb
