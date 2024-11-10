cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(PHYSICAL_DEBUGGING_CAMERA VERSION 0.0.1)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include/)

find_package(jsoncpp REQUIRED)
find_package(OpenCV REQUIRED
            core imgproc imgcodecs highgui calib3d)

# link_directories(/usr/lib/)

add_subdirectory(app)

add_subdirectory(src)
