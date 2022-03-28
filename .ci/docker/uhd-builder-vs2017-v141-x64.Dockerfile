# Docker build context must be uhd/uhddev root

# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

# This uses Window Server 2019 since it supports container jobs.
# This must match the Windows Server version that the Pipelines
# agent runs on. It's possible to switch out the base image
# with a Windows 10 image for local builds.
FROM mcr.microsoft.com/windows/servercore:1809
LABEL maintainer="Ettus Research"

RUN @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" \
    -NoProfile -InputFormat None -ExecutionPolicy Bypass \
    -Command "[System.Net.ServicePointManager]::SecurityProtocol = 3072; \
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && \
    SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
RUN choco install -y cmake.install --installargs 'ADD_CMAKE_TO_PATH=System' --version=3.22.3
RUN choco install -y doxygen.install --version=1.9.1
RUN choco install -y git
RUN choco install -y NSIS --version=3.06.1
RUN choco install -y vim
RUN choco install -y python3 --version=3.7.9
RUN pip install mako requests numpy ruamel.yaml

RUN powershell -NoProfile -ExecutionPolicy Bypass -Command \
    Invoke-WebRequest "https://aka.ms/vs/15/release/vs_buildtools.exe" \
    -OutFile "%TEMP%\vs_buildtools.exe" -UseBasicParsing
RUN "%TEMP%\vs_buildtools.exe"  --quiet --wait --norestart --noUpdateInstaller \
    --add Microsoft.VisualStudio.Workload.VCTools \
    --add Microsoft.VisualStudio.Component.Windows81SDK \
    --includeRecommended

RUN setx VCPKG_INSTALL_DIR "c:\\vcpkg" /m
RUN git clone https://github.com/microsoft/vcpkg %VCPKG_INSTALL_DIR% && \
    cd %VCPKG_INSTALL_DIR% && \
    # The vcpkg git tag sets the toolchain dependenices
    # This commit uses Boost 1.78 and libusb 1.0.24
    git checkout 2022.03.10 && \
    bootstrap-vcpkg.bat
    # Add custom UHD vcpkg triplet
COPY host/cmake/vcpkg/* c:/vcpkg/triplets/
RUN cd %VCPKG_INSTALL_DIR% && vcpkg install --clean-after-build \
    libusb:uhd-x64-windows-static-md \
    boost:uhd-x64-windows-static-md
