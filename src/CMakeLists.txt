include_directories(${CMAKE_SOURCE_DIR}/include/)

add_library(camera_capture
    camera_capture.cpp
)

target_link_libraries(camera_capture
    PUBLIC
    jsoncpp
    m3api
    ${OpenCV_LIBS}
)
