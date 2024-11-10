include_directories(${CMAKE_SOURCE_DIR}/include/)

add_executable(camera_stream
    camera_stream.cpp
)

target_link_libraries(camera_stream
    PUBLIC
    camera_capture
    jsoncpp
    m3api
    ${OpenCV_LIBS}
)
