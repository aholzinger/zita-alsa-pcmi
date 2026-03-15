# CMakeLists.txt for source directory

option(ZAP_BUILD_SHARED_LIB "Build zita-alsa-pcmi shared library in addition to object library." OFF)

# Find dependencies
find_library(ASOUND_LIB NAMES asound REQUIRED)
message(STATUS "Finding libasound: ${ASOUND_LIB}")

# Set version
set(MAJVERS 0)
set(MINVERS 6.1)
set(VERSION ${MAJVERS}.${MINVERS})

# Library name
set(ZITA_ALSA_PCMI_SO libzita-alsa-pcmi.so)
set(ZITA_ALSA_PCMI_MAJ ${ZITA_ALSA_PCMI_SO}.${MAJVERS})
set(ZITA_ALSA_PCMI_MIN ${ZITA_ALSA_PCMI_MAJ}.${MINVERS})

# Always build object library
add_library(zita-alsa-pcmi-object OBJECT zita-alsa-pcmi.cc zita-alsa-pcmi.h)

# Exclude from default build unless depended on
set_target_properties(zita-alsa-pcmi-object PROPERTIES EXCLUDE_FROM_ALL TRUE)

# Link dependencies
target_link_libraries(zita-alsa-pcmi-object PUBLIC ${ASOUND_LIB})

# Include directories
target_include_directories(zita-alsa-pcmi-object PUBLIC .)

# Compiler flags
target_compile_definitions(zita-alsa-pcmi-object PRIVATE VERSION="${VERSION}" _REENTRANT _POSIX_PTHREAD_SEMANTICS)
target_compile_options(zita-alsa-pcmi-object PRIVATE -O2 -Wall -fPIC -march=native)

if(ZAP_BUILD_SHARED_LIB)
    # Also build shared library using the object files
    add_library(zita-alsa-pcmi-shared SHARED $<TARGET_OBJECTS:zita-alsa-pcmi-object>)

    # Set properties for versioning
    set_target_properties(zita-alsa-pcmi-shared PROPERTIES
        VERSION ${VERSION}
        SOVERSION ${MAJVERS}
        OUTPUT_NAME zita-alsa-pcmi
    )

    # Link dependencies (object library already has asound, but shared needs it too?)
    target_link_libraries(zita-alsa-pcmi-shared PUBLIC ${ASOUND_LIB})

    # Include directories
    target_include_directories(zita-alsa-pcmi-shared PUBLIC .)

    # Compiler flags (already set on objects)
    target_compile_definitions(zita-alsa-pcmi-shared PRIVATE VERSION="${VERSION}" _REENTRANT _POSIX_PTHREAD_SEMANTICS)
    target_compile_options(zita-alsa-pcmi-shared PRIVATE -O2 -Wall -fPIC -march=native)

    # Install rules
    install(TARGETS zita-alsa-pcmi-shared
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    install(FILES zita-alsa-pcmi.h DESTINATION include)
endif()