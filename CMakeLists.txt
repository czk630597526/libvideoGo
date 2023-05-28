cmake_minimum_required(VERSION 3.16.1)


project(Lib_media)
add_compile_options(-fPIC)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

#include_directories(
#        ./include
#        ../include
#        ${CMAKE_SOURCE_DIR})


add_library( # Sets the name of the library.
        media_static
        # Sets the library as a shared library.
        STATIC
        # Provides a relative path to your source file(s).
        libvideo.c
        h264_rtp.c
        libv_log.c
        h264_ps.c
        base64.c
        libv_stat.c
        video_mix.c
        ffm_decoder.c
        h264_decoder.c
        h264_encoder.c
        openh264_encoder.c
        openh264_decoder.c
        pic_sws.c
        )


add_library( # Sets the name of the library.
        media
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        libvideo.c
        h264_rtp.c
        libv_log.c
        h264_ps.c
        base64.c
        libv_stat.c
        video_mix.c
        ffm_decoder.c
        h264_decoder.c
        h264_encoder.c
        openh264_encoder.c
        openh264_decoder.c
        pic_sws.c
        )

target_include_directories(media PUBLIC   ./openh264_inc/ ./ffm_inc_new/ ./include)
target_include_directories(media_static PUBLIC   ./openh264_inc/ ./ffm_inc_new/ ./include)


find_package(SWIG REQUIRED)
include(UseSWIG)
find_program(GO_EXECUTABLE go REQUIRED)

set(CMAKE_SWIG_FLAGS -cgo -intgosize 64)

set_property(SOURCE libvideo.i PROPERTY COMPILE_OPTIONS
        -package libvideo)

swig_add_library(media_go
        LANGUAGE go
        TYPE OBJECT
        OUTPUT_DIR ${PROJECT_BINARY_DIR}/go/${PROJECT_NAME}
        SOURCES libvideo.i)


target_include_directories(media_go PUBLIC  ./openh264_inc/ ./ffm_inc_new/ ./include )
#simplelib.i中有include “*.h”, 这里为这些头文件提供路径

set_target_properties(media_go PROPERTIES
        SWIG_INCLUDE_DIRECTORIES ./include ./openh264_inc/ ./ffm_inc_new/ #SWIG也添加include路径
        SWIG_USE_TARGET_INCLUDE_DIRECTORIES ON
        POSITION_INDEPENDENT_CODE ON)
swig_link_libraries(media_go PUBLIC media)