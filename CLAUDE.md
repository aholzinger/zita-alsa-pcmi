# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`zita-alsa-pcmi` is a C++ library (v0.6.1) that provides easy access to ALSA PCM devices in memory-mapped mode, presenting audio data as single-precision float samples in [-1.0, 1.0]. The core library is a single class (`Alsa_pcmi`) in two files: [source/zita-alsa-pcmi.h](source/zita-alsa-pcmi.h) and [source/zita-alsa-pcmi.cc](source/zita-alsa-pcmi.cc).

Two example applications are included: `alsa_delay` (roundtrip latency measurement) and `alsa_loopback` (software loopback of two channels).

## Build Commands

The primary build system is CMake (minimum 3.24). The original Makefiles in `source/` and `apps/` are preserved for reference only.

**Default build (shared library + apps):**
```bash
cmake -S . -B build
cmake --build build
```

**Common use-case variants:**

```bash
# Library only (for FetchContent integration)
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=OFF -DZAP_BUILD_APPS=OFF -DZAP_USE_SHARED_LIB=OFF
cmake --build build

# Build and install shared library
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=ON -DZAP_BUILD_APPS=OFF -DZAP_USE_SHARED_LIB=OFF
cmake --build build
cmake --install build

# Apps linking to installed shared library
cmake -S . -B build -DZAP_BUILD_SHARED_LIB=OFF -DZAP_BUILD_APPS=ON -DZAP_USE_SHARED_LIB=ON
cmake --build build
```

## CMake Options

| Option | Default | Description |
|---|---|---|
| `ZAP_BUILD_SHARED_LIB` | ON | Build `libzita-alsa-pcmi.so` |
| `ZAP_BUILD_APPS` | ON | Build `alsa_loopback` and `alsa_delay` |
| `ZAP_USE_SHARED_LIB` | ON | Apps link shared lib (local if built, else installed) |

When `ZAP_USE_SHARED_LIB=OFF`, apps link the object library directly (static-like). The unified alias target `zita-alsa-pcmi::zita-alsa-pcmi` always resolves to the appropriate library variant.

## Architecture

- **[source/](source/)**: The library itself — `Alsa_pcmi` class with hardware parameter setup, memory-mapped I/O, format conversion (16/24/32-bit, LE/BE, float), xrun recovery, and poll-based wait.
- **[apps/](apps/)**: Example applications using the library. `pxthread.{h,cc}` is a POSIX thread wrapper shared by both apps. `mtdm.{h,cc}` implements the delay measurement algorithm used by `alsa_delay`.

## Dependencies

- `libasound` (ALSA) — required for the library
- `libpthread` and `librt` — required for the apps

## FetchContent Integration

```cmake
include(FetchContent)
FetchContent_Declare(zita-alsa-pcmi GIT_REPOSITORY <url>)
FetchContent_MakeAvailable(zita-alsa-pcmi)
target_link_libraries(your_target PRIVATE zita-alsa-pcmi::zita-alsa-pcmi)
```

## Debug / Runtime

Enable debug output at runtime by setting the environment variable `ZITA_ALSA_PCMI_DEBUG`, or pass a bitmask to the `Alsa_pcmi` constructor (`DEBUG_INIT=1`, `DEBUG_STAT=2`, `DEBUG_WAIT=4`, `DEBUG_DATA=8`).
