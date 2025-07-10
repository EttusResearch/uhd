# Docker build context must be uhd/uhddev root

# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

FROM mcr.microsoft.com/windows/servercore:ltsc2025
LABEL maintainer="Ettus Research"

ARG PIP_INDEX_HOST=""
ARG PIP_INDEX_URL=""
ENV VCPKG_DISABLE_METRICS=1

RUN setx chocolateyVersion 1.4.0 /m
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
RUN choco install -y python3 --version=3.12.6

# Optionally use cached index.
RUN if defined PIP_INDEX_URL ( \
    pip config --global set global.index-url %PIP_INDEX_URL% && \
    pip config --global set global.trusted-host %PIP_INDEX_HOST% \
    )

RUN pip config list
RUN python -m pip install --upgrade pip
RUN pip install mako requests numpy ruamel.yaml setuptools

RUN powershell -NoProfile -ExecutionPolicy Bypass -Command \
    Invoke-WebRequest "https://aka.ms/vs/17/release/vs_buildtools.exe" \
    -OutFile "%TEMP%\vs_buildtools.exe" -UseBasicParsing
RUN "%TEMP%\vs_buildtools.exe"  --quiet --wait --norestart --noUpdateInstaller \
    --add Microsoft.VisualStudio.Workload.VCTools \
    --includeRecommended

RUN setx VCPKG_INSTALL_DIR "c:\\vcpkg" /m
RUN git clone https://github.com/microsoft/vcpkg %VCPKG_INSTALL_DIR% && \
    cd %VCPKG_INSTALL_DIR% && \
    bootstrap-vcpkg.bat
    # Add custom UHD vcpkg triplet
COPY host/cmake/vcpkg/* c:/vcpkg/triplets/
RUN mkdir c:\\uhd-vcpkg
COPY .ci\\docker\\vcpkg\\vcpkg.json c:/uhd-vcpkg/vcpkg.json
RUN cd c:\\uhd-vcpkg && %VCPKG_INSTALL_DIR%\vcpkg.exe install \
    --triplet uhd-x64-windows-static-md \
    --clean-after-build
