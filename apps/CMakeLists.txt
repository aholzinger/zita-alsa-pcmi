# CMakeLists.txt for apps directory

# Find dependencies
find_library(PTHREAD_LIB NAMES pthread REQUIRED)
message(STATUS "Finding libpthread: ${PTHREAD_LIB}")

find_library(RT_LIB NAMES rt REQUIRED)
message(STATUS "Finding librt: ${RT_LIB}")

# Set version
set(VERSION "0.3.2")

# Compiler flags
add_compile_definitions(VERSION="${VERSION}")
add_compile_options(-O2 -Wall)

# alsa_loopback executable
add_executable(alsa_loopback alsa_loopback.cc pxthread.cc)
target_include_directories(alsa_loopback PRIVATE ../source)
target_link_libraries(alsa_loopback PRIVATE zita-alsa-pcmi::zita-alsa-pcmi ${PTHREAD_LIB} ${RT_LIB})

# alsa_delay executable
add_executable(alsa_delay alsa_delay.cc mtdm.cc pxthread.cc)
target_include_directories(alsa_delay PRIVATE ../source)
target_link_libraries(alsa_delay PRIVATE zita-alsa-pcmi::zita-alsa-pcmi ${PTHREAD_LIB} ${RT_LIB})