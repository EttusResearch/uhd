# Docker build context must be uhd/uhddev root

# please follow docker best practices
# https://docs.docker.com/engine/userguide/eng-image/dockerfile_best-practices/

# Defaults for variables used in FROM statement
ARG WIN_BASE_TAG=ltsc2025

FROM mcr.microsoft.com/windows/servercore:${WIN_BASE_TAG}
LABEL maintainer="Ettus Research"

# Minimum required argument
ARG VS_BUILD_TOOLS_URL
# Optional arguments for customization
ARG PYTHON_VERSION=3.10.11
ARG VCPKG_MANIFEST_FILE=vcpkg-vs2026.json
ARG PIP_INDEX_HOST=""
ARG PIP_INDEX_URL=""
ENV VCPKG_DISABLE_METRICS=1

# Enable long file paths (>260 characters)
RUN reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystem /v LongPathsEnabled /t REG_DWORD /d 1 /f

RUN setx chocolateyVersion 2.5.1 /m
RUN @"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" \
    -NoProfile -InputFormat None -ExecutionPolicy Bypass \
    -Command "[System.Net.ServicePointManager]::SecurityProtocol = 3072; \
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && \
    SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
RUN choco install -y cmake.install --installargs 'ADD_CMAKE_TO_PATH=System' --version=4.2.1
RUN choco install -y doxygen.install --version=1.9.8
RUN choco install -y git
RUN choco install -y NSIS --version=3.11.0
RUN choco install -y vim
RUN choco install -y python3 --version=%PYTHON_VERSION%

COPY .ci/docker/scripts/check-url.ps1 C:/Temp/check-url.ps1
RUN powershell -NoProfile -ExecutionPolicy Bypass -File C:\Temp\check-url.ps1 \
    -Url "%VS_BUILD_TOOLS_URL%"
RUN powershell -NoProfile -ExecutionPolicy Bypass -Command \
    Invoke-WebRequest "%VS_BUILD_TOOLS_URL%" \
    -OutFile "%TEMP%\vs_buildtools.exe" -UseBasicParsing
RUN "%TEMP%\vs_buildtools.exe"  --quiet --wait --norestart --noUpdateInstaller \
    --add Microsoft.VisualStudio.Workload.VCTools \
    --includeRecommended

# Optionally use cached index.
RUN if defined PIP_INDEX_URL ( \
    pip config --global set global.index-url %PIP_INDEX_URL% && \
    pip config --global set global.trusted-host %PIP_INDEX_HOST% \
    )

COPY host/python/requirements.txt C:/Temp/requirements.txt
RUN pip config list
RUN python -m pip install --upgrade pip
RUN pip install -r C:/Temp/requirements.txt

RUN setx VCPKG_INSTALL_DIR "c:\\vcpkg" /m
RUN git clone https://github.com/microsoft/vcpkg %VCPKG_INSTALL_DIR% && \
    cd %VCPKG_INSTALL_DIR% && \
    bootstrap-vcpkg.bat
    # Add custom UHD vcpkg triplet
COPY host/cmake/vcpkg/* c:/vcpkg/triplets/
RUN mkdir c:\\uhd-vcpkg
COPY .ci/docker/vcpkg/${VCPKG_MANIFEST_FILE} c:/uhd-vcpkg/vcpkg.json
RUN cd c:\\uhd-vcpkg && %VCPKG_INSTALL_DIR%\vcpkg.exe install \
    --triplet uhd-x64-windows-static-md \
    --clean-after-build
