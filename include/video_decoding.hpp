#pragma once

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libavutil/imgutils.h>
  #include <libavutil/opt.h>
}

#include <jsoncpp/json/json.h>
#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

// Socket streaming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class VideoDecoding {
public:
  VideoDecoding(Json::Value jsonVideoConf,
                const std::string& output_file, 
                int session_idx,
                int socket);

  ~VideoDecoding();

  void initialize_ffmpeg_decoder();

  void decode_frame(cv::Mat* decoded_frame);

  AVCodecContext* get_codec_ctx();

  AVFormatContext* get_format_ctx();

  void swapRGBToBGR(cv::Mat* image);

  void convertNV12ToBGR(const AVFrame* frame_nv12, cv::Mat* bgr);

private:
  const AVCodec* codec_ = nullptr;
  AVCodecContext* codec_ctx_ = nullptr;
  AVFormatContext* format_ctx_ = nullptr;

  std::string decoder_name_;

  AVPacket* pkt_ = nullptr;

  AVFrame* frame_nv12_ = nullptr;
  std::vector<uint8_t>* buffer_ = nullptr;

  int session_idx_ = -1;

  int width_ = 0;
  int height_ = 0;
  int frame_rate_ = 0;
  int bitrate_ = 0;
  int gop_size_ = 0;
  std::string output_file_;

  int socket_ = -1;
};
