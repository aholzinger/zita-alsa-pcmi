# zita-alsa-pcmi

A C++ library for ALSA PCM I/O with high precision timing and low latency.

Original project authors, see file AUTHORS. See README for original project
description.

CMake build support added by Axel Holzinger and Stefan Heinzmann. 

## Build System

This project uses CMake with a flexible build system supporting multiple
configurations.

### Options

- `ZAP_BUILD_SHARED_LIB` (default: ON): Build the shared library in addition to
  the object library. When ON, creates `libzita-alsa-pcmi.so` that can be
  installed.
- `ZAP_BUILD_APPS` (default: ON): Build the example applications
  (`alsa_loopback` and `alsa_delay`).
- `ZAP_USE_SHARED_LIB` (default: ON): For applications, use the shared library
  instead of direct object inclusion. When ON and `ZAP_BUILD_SHARED_LIB` is ON,
  uses the locally built shared lib; otherwise, uses an installed shared lib.

### Use Cases

#### 1. Library Only (for FetchContent)
```bash
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=OFF -DZAP_BUILD_APPS=OFF -DZAP_USE_SHARED_LIB=OFF
cmake --build build
```
Define the object library target for direct inclusion with FetchContent. Only
builds the object library when actually used.

#### 2. Build and Install Shared Library
```bash
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=ON -DZAP_BUILD_APPS=OFF -DZAP_USE_SHARED_LIB=OFF
cmake --build build
cmake --install build
```
Creates and installs the shared library and headers.

#### 3. Build Applications with Local Library
```bash
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=OFF -DZAP_BUILD_APPS=ON -DZAP_USE_SHARED_LIB=OFF
cmake --build build
```
Builds applications with the library sources included directly (static-like
linking).

#### 4. Build Applications with Local Shared Library
```bash
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=ON -DZAP_BUILD_APPS=ON -DZAP_USE_SHARED_LIB=ON
cmake --build build
```
Builds both shared library and applications linking to it.

#### 5. Build Applications with Installed Shared Library
```bash
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=OFF -DZAP_BUILD_APPS=ON -DZAP_USE_SHARED_LIB=ON
cmake --build build
```
Builds applications linking to an installed shared library.

### FetchContent Usage

To use this library in another CMake project:

```cmake
include(FetchContent)

FetchContent_Declare(
    zita-alsa-pcmi
    GIT_REPOSITORY https://github.com/your-repo/zita-alsa-pcmi.git
)
FetchContent_MakeAvailable(zita-alsa-pcmi)

# Link to the library
target_link_libraries(your_target PRIVATE zita-alsa-pcmi::zita-alsa-pcmi)
```

This provides direct source inclusion for optimal linking.

### Dependencies

- ALSA library (libasound)
- POSIX threads (libpthread)
- Real-time library (librt)

### Installation

The shared library installs to standard system locations:
- Libraries: `/usr/local/lib`
- Headers: `/usr/local/include`

### Original Makefiles

The original Makefile-based build system is preserved in the `apps/` and
`source/` directories for reference.