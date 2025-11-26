# CLAUDE.md - UHD Repository Guide for AI Assistants

This document provides guidance for AI assistants working with the UHD (USRP Hardware Driver) codebase.

## Project Overview

UHD is the free and open-source software driver and API for the Universal Software Radio Peripheral (USRP) SDR platform, created and sold by Ettus Research (a National Instruments brand). UHD supports all Ettus Research USRP hardware including motherboards, daughterboards, and their combinations.

## Repository Structure

```
uhd/
├── host/           # User-space driver and API (main development area)
│   ├── include/    # Public API headers (uhd/*.hpp)
│   ├── lib/        # Core library implementation
│   │   ├── usrp/   # Device-specific implementations (b100, b200, x300, x400, mpmd)
│   │   ├── rfnoc/  # RFNoC (RF Network on Chip) framework
│   │   ├── transport/ # Data transport layers
│   │   └── deps/   # Bundled dependencies (flatbuffers, pybind11, rpclib)
│   ├── examples/   # Example applications (rx_samples_to_file, tx_waveforms, etc.)
│   ├── tests/      # Unit tests and device tests
│   ├── utils/      # Utility programs (uhd_find_devices, uhd_usrp_probe)
│   ├── python/     # Python bindings
│   └── docs/       # Documentation sources
├── mpm/            # Module Peripheral Manager (embedded device code)
│   ├── lib/        # MPM libraries
│   ├── python/     # Python MPM implementation
│   └── systemd/    # Systemd service files
├── fpga/           # FPGA HDL source code
│   ├── usrp1/      # Generation 1 (USRP Classic)
│   ├── usrp2/      # Generation 2 (N2X0, B100, E1X0, USRP2)
│   └── usrp3/      # Generation 3 (B2X0, X Series, E3X0, N3xx, X4xx)
├── firmware/       # Microprocessor firmware
├── images/         # FPGA/firmware image packaging tools
├── tools/          # Development and debugging tools
└── .ci/            # CI/CD pipeline configurations
```

## Build System

### Host Library (CMake)

Build commands from `host/` directory:
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
make test        # Run unit tests
make install     # Install to system
```

Key CMake options:
- `-DENABLE_TESTS=ON` - Enable unit tests (default ON)
- `-DENABLE_EXAMPLES=ON` - Enable example programs (default ON)
- `-DENABLE_PYTHON_API=ON` - Enable Python bindings (default ON on Linux)
- `-DENABLE_C_API=ON` - Enable C API (default ON)
- `-DCMAKE_BUILD_TYPE=Release` - Build type (Release default)

### Dependencies

Minimum versions (from `host/cmake/Modules/UHDMinDepVersions.cmake`):
- CMake: 3.12
- GCC: 7.3.0 / Clang: 6.0.0 / MSVC: 15.0
- Python: 3.7
- Boost: 1.71
- C++ Standard: C++20

Required Boost components: chrono, date_time, filesystem, program_options, serialization, thread, unit_test_framework

Python dependencies: mako, requests, numpy, ruamel.yaml

## Coding Standards

### C++ Guidelines

- **Formatting**: Use `clang-format` with the `.clang-format` file in repo root
- **Indentation**: 4 spaces, never tabs
- **Line length**: 79 chars preferred, 90 chars max
- **Include order**: Local headers → UHD headers → 3rd-party → Boost → Standard
- **Prefer standard C++ over Boost** where equivalent functionality exists
- **Use `std::chrono`** instead of Boost sleep functions
- **Use `.at()` over `[]`** for maps and vectors to get exceptions on invalid access
- **Use size-specific types** (`int32_t`, etc.) when interfacing with hardware

Example include order:
```cpp
#include "x300_specific.hpp"
#include <uhd/utils/log.hpp>
#include <libusb/foo.hpp>
#include <boost/shared_ptr.hpp>
#include <mutex>
```

### Python Guidelines

- Python 3 only (Python 2 not supported since UHD 4.0)
- Follow NI Python Style Guidelines
- Use `ni-python-styleguide` for linting/formatting
- Use `black` and `isort` for formatting

### FPGA/Verilog Guidelines (in `fpga/CODING.md`)

- **Indentation**: 2 spaces, never tabs
- Use `begin`/`end` for code blocks
- Sequential blocks: non-blocking assignments (`<=`)
- Combinational blocks: blocking assignments (`=`)
- One module per file, filename matches module name
- Use `default_nettype none` at start, `default_nettype wire` at end

## Testing

### Unit Tests (C++)

Located in `host/tests/`. Run with:
```bash
cd build
make test
# Or run specific test:
./tests/ranges_test
```

Tests use Boost.Test framework with `BOOST_TEST_DYN_LINK`.

### Python Tests

```bash
cd build
ctest -R py  # Run Python tests
```

### Device Tests

Located in `host/tests/devtest/`. These require physical USRP hardware:
```bash
python -m pytest host/tests/devtest/
```

## Supported Devices

Device support is modular via `LIBUHD_REGISTER_COMPONENT`:

| Device | CMake Flag | Transport |
|--------|------------|-----------|
| B100 | `ENABLE_B100` | USB |
| B200/B210 | `ENABLE_B200` | USB |
| USRP1 | `ENABLE_USRP1` | USB |
| USRP2/N2X0 | `ENABLE_USRP2` | Ethernet |
| X300/X310 | `ENABLE_X300` | Ethernet/PCIe |
| N300/N310 | `ENABLE_N300` | MPM (Ethernet) |
| N320/N321 | `ENABLE_N320` | MPM (Ethernet) |
| E320 | `ENABLE_E320` | MPM (Embedded) |
| E310/E312 | `ENABLE_E300` | MPM (Embedded) |
| X410/X440 | `ENABLE_X400` | MPM (Ethernet) |

## Key Components

### RFNoC (RF Network on Chip)

RFNoC is UHD's FPGA framework for signal processing. Key files:
- `host/lib/rfnoc/` - Block controllers and graph management
- `host/include/uhd/rfnoc/` - Public RFNoC API
- `fpga/usrp3/lib/rfnoc/` - FPGA block implementations

### multi_usrp API

The primary user-facing API for controlling USRPs:
- `host/lib/usrp/multi_usrp.cpp` - Classic API implementation
- `host/lib/usrp/multi_usrp_rfnoc.cpp` - RFNoC-based implementation

### MPM (Module Peripheral Manager)

Runs on embedded Linux devices (N3xx, E3xx, X4xx):
- `mpm/python/usrp_mpm/` - Python MPM implementation
- Communicates with host via RPC

## Git Commit Guidelines

- Use fast-forward merges, avoid merge commits
- Prefix subject with affected section: `x300: Fix overflow at full moon`
- Subject: 50 chars ideal, 72 max
- Imperative mood in subject line
- Body: explain *what* and *why*, wrap at 72 chars
- Separate refactoring/cleanup commits from behavior changes

## Pre-commit Hooks

Install with:
```bash
pip install pre-commit ni-python-styleguide
pre-commit install
```

Runs `clang-format` (C++) and `ni-python-styleguide` (Python) before commits.

## Common Tasks

### Find devices
```bash
uhd_find_devices
uhd_usrp_probe --args="addr=192.168.10.2"
```

### Run an example
```bash
./examples/rx_samples_to_file --args="addr=192.168.10.2" --file=output.dat
```

### Download FPGA images
```bash
uhd_images_downloader
```

## File Naming Conventions

- C++ sources: `snake_case.cpp`, `snake_case.hpp`
- Python: `snake_case.py`
- Verilog: Module name matches filename
- Tests: `*_test.cpp`, `*_test.py`

## Important Files

- `CODING.md` - Detailed coding standards
- `CONTRIBUTING.md` - Contribution guidelines
- `fpga/CODING.md` - FPGA-specific coding standards
- `.clang-format` - C++ formatting rules
- `.pre-commit-config.yaml` - Pre-commit hook configuration
- `host/cmake/Modules/UHDMinDepVersions.cmake` - Dependency versions

## License

- Host code: GPL-3.0-or-later
- FPGA code: LGPL-3.0-or-later

## External Resources

- [UHD Manual](http://files.ettus.com/manual/)
- [Knowledge Base](https://kb.ettus.com)
- [GitHub Issues](https://github.com/EttusResearch/uhd/issues)
- [USRP Users Mailing List](https://lists.ettus.com/list/usrp-users.lists.ettus.com)
