# Top-level CMakeLists.txt for zita-alsa-pcmi
cmake_minimum_required(VERSION 3.24)
project(zita-alsa-pcmi)

find_library(ZAP_ASOUND_LIB NAMES asound)
message(STATUS "Finding libasound: ${ZAP_ASOUND_LIB}")

find_library(ZAP_PTHREAD_LIB NAMES pthread)
message(STATUS "Finding libpthread: ${ZAP_PTHREAD_LIB}")

find_library(ZAP_RT_LIB NAMES rt)
message(STATUS "Finding librt: ${ZAP_RT_LIB}")

option(ZAP_BUILD_LIB "Build zita-alsa-pcmi library." ON)
if(ZAP_BUILD_LIB)
    message(STATUS "Building library zita-alsa-pcmi.")
    
    # Create zita-alsa-pcmi library
    add_library(zita-alsa-pcmi STATIC source/zita-alsa-pcmi.cc source/zita-alsa-pcmi.h)
    target_link_libraries(zita-alsa-pcmi PUBLIC ${ZAP_ASOUND_LIB})

    # Include directory
    target_include_directories(zita-alsa-pcmi PUBLIC source)

    # alsa_delay application
    add_executable(alsa_delay apps/alsa_delay.cc apps/mtdm.cc apps/pxthread.cc)

    # Link system libs and zita-alsa-pcmi library to the alsa_delay executable
    target_link_libraries(alsa_delay PUBLIC ${ZAP_PTHREAD_LIB} ${ZAP_RT_LIB} PRIVATE zita-alsa-pcmi)

    # alsa_loopback application
    add_executable(alsa_loopback apps/alsa_loopback.cc apps/pxthread.cc)

    # Link Link system libs and zita-alsa-pcmi library to the alsa_loopback executable
    target_link_libraries(alsa_loopback PUBLIC ${ZAP_PTHREAD_LIB} ${ZAP_RT_LIB} PRIVATE zita-alsa-pcmi)
else()
    message(STATUS "Not building library zita-alsa-pcmi.")
    
    # alsa_delay application
    add_executable(alsa_delay apps/alsa_delay.cc apps/mtdm.cc apps/pxthread.cc source/zita-alsa-pcmi.cc)

    # Include directory
    target_include_directories(alsa_delay PUBLIC source)

    # Link Link system libs to the alsa_delay executable
    target_link_libraries(alsa_delay PUBLIC ${ZAP_ASOUND_LIB} ${ZAP_PTHREAD_LIB} ${ZAP_RT_LIB})

    # alsa_loopback application
    add_executable(alsa_loopback apps/alsa_loopback.cc apps/pxthread.cc source/zita-alsa-pcmi.cc)

    # Include directory
    target_include_directories(alsa_loopback PUBLIC source)

    # Link Link system libs to the alsa_loopback executable
    target_link_libraries(alsa_loopback PUBLIC ${ZAP_ASOUND_LIB} ${ZAP_PTHREAD_LIB} ${ZAP_RT_LIB})
endif()
