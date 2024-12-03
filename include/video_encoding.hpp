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
#include <thread>
#include <vector>

// Socket streaming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class VideoEncoding {
public:
  AVFrame* frame_nv12 = nullptr;

  VideoEncoding(Json::Value jsonVideoConf,
                const std::string& output_file, 
                int session_idx,
                int socket);

  ~VideoEncoding();

  void initialize_ffmpeg_encoder(bool write_to_file);

  void encode_frame_to_file(cv::Mat* frame,
                            int64_t frame_count);

  void convertBGRAtoNV12(const cv::Mat* bgra);

  void encode_frame_to_stream(cv::Mat* frame,
                              int64_t frame_count);

  AVCodecContext* get_codec_ctx();

  AVFormatContext* get_format_ctx();

  int get_width();

  int get_height();

private:
  const AVCodec* codec_ = nullptr;
  AVCodecContext* codec_ctx_ = nullptr;
  AVFormatContext* format_ctx_ = nullptr;
  AVPacket* pkt_ = nullptr;

  std::string encoder_name_;
  std::string preset_;
  std::string tune_;
  std::string split_encode_mode_;

  int session_idx_ = -1;

  std::vector<std::thread> encoding_threads_;

  int width_ = 0;
  int height_ = 0;
  int frame_rate_ = 0;
  int bitrate_ = 0;
  int gop_size_ = 0;
  std::string output_file_;

  int socket_ = -1;
};
