cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(PHYSICAL_DEBUGGING_CAMERA VERSION 0.0.1)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include/)

find_package(jsoncpp REQUIRED)
find_package(OpenCV REQUIRED
            core imgproc imgcodecs highgui calib3d)


set(FFMPEG_ROOT /usr/local/bin/ffmpeg)
set(CMAKE_PREFIX_PATH ${FFMPEG_ROOT})
set(FFMPEG_DEP_LIB /usr/local/lib)

# Get FFmpeg version and print it in the CMake log
execute_process(
    COMMAND ${FFMPEG_EXECUTABLE} -version
    OUTPUT_VARIABLE FFMPEG_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Found FFmpeg version: ${FFMPEG_VERSION}")

find_library(LIBYUV_LIB yuv PATHS /usr/local/lib REQUIRED)

find_library(AVCODEC_LIBRARIES avcodec HINTS ${FFMPEG_DEP_LIB}/lib REQUIRED)
find_library(AVFORMAT_LIBRARIES avformat HINTS ${FFMPEG_DEP_LIB}/lib REQUIRED)
find_library(AVUTIL_LIBRARIES avutil HINTS ${FFMPEG_DEP_LIB}/lib REQUIRED)
include_directories(${FFMPEG_DEP_LIB}/include)

add_subdirectory(app)

add_subdirectory(src)
