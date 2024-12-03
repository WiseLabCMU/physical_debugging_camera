include_directories(${CMAKE_SOURCE_DIR}/include/)

add_executable(camera_stream
    camera_stream.cpp
)

target_link_libraries(camera_stream
    PUBLIC
    camera_capture
    video_encoding
    jsoncpp
    m3api
    yuv
    ${OpenCV_LIBS}
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVUTIL_LIBRARIES}
)
