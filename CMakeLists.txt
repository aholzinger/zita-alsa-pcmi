# Top-level CMakeLists.txt for zita-alsa-pcmi
cmake_minimum_required(VERSION 3.24)
project(zita-alsa-pcmi)

option(ZAP_BUILD_APPS "Build the applications." OFF)
option(ZAP_USE_SHARED_LIB "Use shared library (local if built, else installed) instead of object library." OFF)

add_subdirectory(source)

# Create unified alias target
if(ZAP_USE_SHARED_LIB)
    if(ZAP_BUILD_SHARED_LIB)
        add_library(zita-alsa-pcmi::zita-alsa-pcmi ALIAS zita-alsa-pcmi-shared)
    else()
        # Use installed shared library
        find_library(ZITA_ALSA_PCMI_LIB NAMES zita-alsa-pcmi REQUIRED)
        find_path(ZITA_ALSA_PCMI_INCLUDE_DIR NAMES zita-alsa-pcmi.h REQUIRED)
        add_library(zita-alsa-pcmi::zita-alsa-pcmi UNKNOWN IMPORTED)
        set_target_properties(zita-alsa-pcmi::zita-alsa-pcmi PROPERTIES
            IMPORTED_LOCATION ${ZITA_ALSA_PCMI_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${ZITA_ALSA_PCMI_INCLUDE_DIR}
        )
    endif()
else()
    add_library(zita-alsa-pcmi::zita-alsa-pcmi ALIAS zita-alsa-pcmi-object)
endif()

if(ZAP_BUILD_APPS)
    add_subdirectory(apps)
endif()
