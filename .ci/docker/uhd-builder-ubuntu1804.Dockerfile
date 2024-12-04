# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

FROM ubuntu:18.04
LABEL maintainer="Ettus Research"

# This will make apt-get install without question
ARG DEBIAN_FRONTEND=noninteractive

# This is a workaround for Ubuntu 18.04 systemd bug #1988563
# https://bugs.launchpad.net/ubuntu/+source/systemd/+bug/1988563
# When https://bugs.launchpad.net/ubuntu/+source/systemd/+bug/1988119
# is resolved with a new systemd version, this should be removed.
ADD http://archive.ubuntu.com/ubuntu/pool/main/s/systemd/libudev1_237-3ubuntu10_amd64.deb libudev1_237-3ubuntu10_amd64.deb
RUN dpkg -i libudev1_237-3ubuntu10_amd64.deb
ADD http://archive.ubuntu.com/ubuntu/pool/main/s/systemd/libudev-dev_237-3ubuntu10_amd64.deb libudev-dev_237-3ubuntu10_amd64.deb
RUN dpkg -i libudev-dev_237-3ubuntu10_amd64.deb

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
        doxygen \
        dpdk \
        libdpdk-dev \
        libgps-dev \
        libgps-dev \
        libudev-dev \
        libusb-1.0-0-dev \
        ncompress \
        ninja-build \
        python3.7 \
        python3.7-dev \
        python3.7-venv \
        python3-pip \
        python3-requests \
        pybind11-dev \
    # Install deb dependencies
        debootstrap \
        devscripts \
        pbuilder \
        debhelper \
        libncurses5-dev \
    # Install GNURadio dependencies
        libsdl1.2-dev \
        libgsl-dev \
        libqwt-qt5-dev \
        libqt5opengl5-dev \
        libgmp3-dev \
        libfftw3-dev \
        swig \
        gir1.2-gtk-3.0 \
        libpango1.0-dev \
        liblog4cpp5-dev \
        libzmq3-dev \
        && \
    rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.6 5 && \
    update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.7 10 && \
    python3 -m pip install --upgrade pip

RUN python3 -m pip install \
    # Install UHD python dependencies
        docutils \
        mako \
        numpy \
        requests \
    # Install deb python dependencies
        ruamel.yaml \
    # Install GNURadio dependencies
        sphinx \
        lxml \
        pyqt5 \
        click \
        click-plugins \
        zmq \
        scipy

RUN wget https://cmake.org/files/v3.12/cmake-3.12.4-Linux-x86_64.tar.gz -O /tmp/cmake.tar.gz && \
        (echo "486edd6710b5250946b4b199406ccbf8f567ef0e23cfe38f7938b8c78a2ffa5f  /tmp/cmake.tar.gz" | sha256sum --check --status ) && \
        tar -zxvf /tmp/cmake.tar.gz -C /opt && \
        cp -r /opt/cmake-3.12.4-Linux-x86_64/bin/* /usr/local/bin/ && \
        cp -r /opt/cmake-3.12.4-Linux-x86_64/share/* /usr/local/share/ && \
        rm /tmp/cmake.tar.gz


RUN wget https://archives.boost.io/release/1.66.0/source/boost_1_66_0.tar.bz2 -O /tmp/boost.tar.bz2 && \
        (echo "5721818253e6a0989583192f96782c4a98eb6204965316df9f5ad75819225ca9 /tmp/boost.tar.bz2" | sha256sum --check --status ) && \
        tar -jxf /tmp/boost.tar.bz2 -C /tmp && \
        cd /tmp/boost_1_66_0 && \
        ./bootstrap.sh --prefix=/usr --with-libraries=atomic,date_time,exception,filesystem,graph,math,program_options,regex,serialization,stacktrace,signals,system,test,thread,timer && \
        ./b2 install && \
        rm -rf /tmp/boost.tar.bz2 /tmp/boost_1_66_0
