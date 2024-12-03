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

add_library(video_encoding
    video_encoding.cpp
    network_connection.cpp
)

target_link_libraries(video_encoding
    PUBLIC
    yuv
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${OpenCV_LIBS}
)

add_library(video_decoding
    video_decoding.cpp
    network_connection.cpp
)

target_link_libraries(video_decoding
    PUBLIC
    yuv
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${OpenCV_LIBS}
)
