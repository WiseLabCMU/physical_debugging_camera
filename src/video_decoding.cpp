#include "video_decoding.hpp"
#include "network_connection.hpp"

#include <jsoncpp/json/json.h>
#include <opencv2/opencv.hpp>
#include <libyuv.h>

#include <string>
#include <thread>
#include <vector>
#include <iostream>

// Socket streaming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

VideoDecoding::VideoDecoding(Json::Value jsonVideoConf,
                              const std::string& output_file, 
                              int session_idx,
                              int socket) {
  av_log_set_level(AV_LOG_VERBOSE);

  this->session_idx_ = session_idx;

  std::cout << "Using decoder: " << jsonVideoConf["decoder"].asString() << std::endl;
  this->decoder_name_ = jsonVideoConf["decoder"].asString();
  
  this->width_ = jsonVideoConf["stream_width"].asInt();
  this->height_ = jsonVideoConf["stream_height"].asInt();
  this->frame_rate_ = jsonVideoConf["frame_rate"].asInt();
  this->output_file_ = jsonVideoConf["output_video_path"].asString() + "_" + 
                        std::to_string(this->session_idx_) + ".mp4";

  this->socket_ = socket;

  this->frame_nv12_ = av_frame_alloc();  // Frame to hold the YUV image
  if (!this->frame_nv12_)
  {
    std::cerr << "Could not allocate AVFrame\n";
    exit(1);
  }

  int buffer_size = jsonVideoConf["pre_allocated_buffer_size"].asInt();
  this->buffer_ = new std::vector<uint8_t>(buffer_size);

  this->pkt_ = av_packet_alloc();
  if (!this->pkt_)
  {
    std::cerr << "Could not allocate AVPacket\n";
    return;
  }

  initialize_ffmpeg_decoder();
}

VideoDecoding::~VideoDecoding() {
  avcodec_free_context(&this->codec_ctx_);
  avformat_free_context(this->format_ctx_);
  av_packet_free(&this->pkt_);
}

// Initialize FFmpeg Decoder
void VideoDecoding::initialize_ffmpeg_decoder() {
  this->codec_ = avcodec_find_decoder_by_name(decoder_name_.c_str());
  if (!this->codec_) 
  {
    std::cerr << "Requested codec not found!" << std::endl;
    exit(1);
  }

  this->codec_ctx_ = avcodec_alloc_context3(this->codec_);
  if (avcodec_open2(this->codec_ctx_, this->codec_, NULL) < 0)
  {
    std::cerr << "Could not open codec." << std::endl;
    avcodec_free_context(&(this->codec_ctx_));
    exit(1);
  }

  this->codec_ctx_->time_base = (AVRational){1, this->frame_rate_};
  this->codec_ctx_->framerate = (AVRational){this->frame_rate_, 1};
  this->codec_ctx_->pix_fmt = AV_PIX_FMT_NV12;

  std::cout << "Decoder " << this->decoder_name_<< " initialized." << std::endl;
}

void VideoDecoding::swapRGBToBGR(cv::Mat* image) {
  // Ensure the input is a 3-channel image
  if (image->channels() != 3) 
  {
      std::cerr << "Image must have 3 channels for RGB to BGR conversion." << std::endl;
      return;
  }

  // Iterate through each pixel and swap the R and B channels
  int rows = image->rows;
  int cols = image->cols * 3; // 3 channels per pixel

  // Process row by row
  for (int i = 0; i < rows; ++i) 
  {
    uint8_t* row = image->ptr<uint8_t>(i);
    for (int j = 0; j < cols; j += 3) 
    {
      std::swap(row[j], row[j + 2]); // Swap R and B
    }
  }
}

void VideoDecoding::convertNV12ToBGR(const AVFrame* frame_nv12, cv::Mat* bgr) {
  // Ensure the input format is NV12
  if (frame_nv12->format != AV_PIX_FMT_NV12)
  {
    std::cerr << "Invalid format. Expected NV12!" << std::endl;
    return;
  }

  // Use libyuv to convert NV12 to BGR24 directly
  int ret = libyuv::NV12ToRGB24(
    frame_nv12->data[0], frame_nv12->linesize[0],      // Y plane and stride
    frame_nv12->data[1], frame_nv12->linesize[1],      // UV plane and stride
    bgr->data, frame_nv12->width * 3,                  // BGR buffer and stride
    frame_nv12->width, frame_nv12->height              // Frame width and height
  );

  if (ret != 0) 
  {
    std::cerr << "libyuv NV12ToRGB24 failed with error: " << ret << std::endl;
  }
}

void VideoDecoding::decode_frame(cv::Mat* decoded_frame) {  
  // Receive packet size
  int pkt_size;
  if (recv(this->socket_, &pkt_size, sizeof(int), 0) <= 0)
  {
    std::cerr << "Failed to receive packet size." << std::endl;
  }

  // Allocate packet data
  if (av_new_packet(this->pkt_, pkt_size) != 0) 
  {
    std::cerr << "Could not allocate packet." << std::endl;
    return;
  }

  // Receive packet data directly into the allocated packet buffer
  if (!receive_all(this->socket_, reinterpret_cast<char*>(this->pkt_->data), pkt_size))
  {
    std::cerr << "Failed to receive packet data." << std::endl;
    av_packet_unref(this->pkt_);
    return;
  }

  if (avcodec_send_packet(this->codec_ctx_, this->pkt_) < 0) 
  {
    std::cerr << "Error sending packet for decoding." << std::endl;
    av_packet_unref(this->pkt_);
    return;
  }

  if (avcodec_receive_frame(this->codec_ctx_, this->frame_nv12_) == 0) 
  {
    convertNV12ToBGR(this->frame_nv12_, decoded_frame);
  }
  else 
  {
    std::cerr << "Error receiving frame." << std::endl;
  }

  av_packet_unref(this->pkt_);
}

AVCodecContext* VideoDecoding::get_codec_ctx() {
  return this->codec_ctx_;
}

AVFormatContext* VideoDecoding::get_format_ctx() {
  return this->format_ctx_;
}
