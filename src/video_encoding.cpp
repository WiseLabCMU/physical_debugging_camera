#include "video_encoding.hpp"
#include "network_connection.hpp"

#include <jsoncpp/json/json.h>
#include <opencv2/opencv.hpp>
#include <libyuv.h>

#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

// Socket streaming
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

VideoEncoding::VideoEncoding(Json::Value jsonVideoConf,
                              const std::string& output_file, 
                              int session_idx,
                              int socket) {
  av_log_set_level(AV_LOG_VERBOSE);

  this->session_idx_ = session_idx;

  std::cout << "Using encoder: " << jsonVideoConf["encoder"].asString() << std::endl;
  this->encoder_name_ = jsonVideoConf["encoder"].asString();
  
  this->width_ = jsonVideoConf["stream_width"].asInt();
  this->height_ = jsonVideoConf["stream_height"].asInt();
  this->frame_rate_ = jsonVideoConf["frame_rate"].asInt();
  this->bitrate_ = jsonVideoConf["bitrate"].asInt();
  this->gop_size_ = jsonVideoConf["gop_size"].asInt();
  this->output_file_ = jsonVideoConf["output_video_path"].asString() + "_" + 
                        std::to_string(this->session_idx_) + ".mp4";

  this->preset_ = jsonVideoConf["preset"].asString();
  this->tune_ = jsonVideoConf["tune"].asString();
  this->split_encode_mode_ = jsonVideoConf["split_encode_mode"].asString();

  this->socket_ = socket;

  initialize_ffmpeg_encoder(true);

  this->pkt_ = av_packet_alloc();
  if (!this->pkt_) 
  {
    fprintf(stderr, "Could not allocate AVPacket\n");
    exit(1);
  }

  this->frame_nv12 = av_frame_alloc();
  if (!this->frame_nv12)
  {
    fprintf(stderr, "Could not allocate AVFrame for YUV\n");
    exit(1);
  }
  this->frame_nv12->format = AV_PIX_FMT_NV12;
  this->frame_nv12->width = this->width_;
  this->frame_nv12->height = this->height_;
  int result = av_image_alloc(this->frame_nv12->data, 
                              this->frame_nv12->linesize, 
                              this->frame_nv12->width, 
                              this->frame_nv12->height, 
                              AV_PIX_FMT_NV12, 
                              32);
  if (result < 0)
  {
    fprintf(stderr, "Could not allocate image for YUV\n");
    exit(1);
  }
}

VideoEncoding::~VideoEncoding() {
  av_freep(&this->frame_nv12->data[0]);
  av_frame_free(&this->frame_nv12);

  avcodec_free_context(&this->codec_ctx_);
  if (this->format_ctx_)
  {
    avformat_free_context(this->format_ctx_);
  }
  av_packet_free(&this->pkt_);
}

void VideoEncoding::initialize_ffmpeg_encoder(bool write_to_file) {
  this->codec_ = avcodec_find_encoder_by_name(this->encoder_name_.c_str());
  if (!this->codec_) 
  {
    std::cerr << "Requested codec not found!" << std::endl;
    exit(1);
  }

  this->codec_ctx_ = avcodec_alloc_context3(this->codec_);

  this->codec_ctx_->bit_rate = this->bitrate_ * 1024 * 1024;
  this->codec_ctx_->width = this->width_;
  this->codec_ctx_->height = this->height_;
  this->codec_ctx_->time_base = (AVRational){1, this->frame_rate_};
  this->codec_ctx_->pkt_timebase = this->codec_ctx_->time_base;
  this->codec_ctx_->framerate = (AVRational){this->frame_rate_, 1};
  this->codec_ctx_->gop_size = this->gop_size_;  // Keyframes interval
  this->codec_ctx_->max_b_frames = 0;  // No B-frames
  this->codec_ctx_->pix_fmt = AV_PIX_FMT_NV12;

  // Zero latency and delay options
  av_opt_set(this->codec_ctx_->priv_data, "preset", this->preset_.c_str(), 0);
  av_opt_set(this->codec_ctx_->priv_data, "tune", this->tune_.c_str(), 0);
  av_opt_set(this->codec_ctx_->priv_data, "split_encode_mode", this->split_encode_mode_.c_str(), 0);

  // These parameters actually adds latency
  // av_opt_set(this->codec_ctx_->priv_data, "rc", "cbr", 0);
  // av_opt_set(this->codec_ctx_->priv_data, "multipass", "fullres", 0);
  // av_opt_set(this->codec_ctx_->priv_data, "zerolatency", "1", 0);
  // av_opt_set_int(this->codec_ctx_->priv_data, "delay", 0, 0);

  // Optional: Set AVDictionary options for lower buffering
  AVDictionary* options = nullptr;
  // if (av_dict_set(&options, "fflags", "nobuffer", 0) < 0)
  // {
  //   fprintf(stderr, "Could not set fflags for AV options\n");
  //   exit(1);
  // }

  if (avcodec_open2(this->codec_ctx_, this->codec_, NULL) < 0)
  {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  // Setting for lossless encoding
  // av_opt_set((this->codec_ctx_)->priv_data, "rc", "constqp", 0);  // Constant QP mode for lossless
  // av_opt_set((this->codec_ctx_)->priv_data, "qp", "0", 0);  // Set QP to 0 for lossless encoding

  if (write_to_file)
  {
    avformat_alloc_output_context2(&(this->format_ctx_), NULL, NULL, this->output_file_.c_str());
    if (!this->format_ctx_)
    {
      fprintf(stderr, "Could not allocate output format context\n");
      exit(1);
    }

    AVStream* stream = avformat_new_stream(this->format_ctx_, this->codec_);
    avcodec_parameters_from_context(stream->codecpar, this->codec_ctx_);
    stream->time_base = this->codec_ctx_->time_base;

    if (!(stream->codecpar->codec_id == AV_CODEC_ID_HEVC))
    {
      fprintf(stderr, "Encoder does not support HEVC\n");
    }

    if (!(stream->codecpar->codec_tag)) 
    {
      avcodec_parameters_from_context(stream->codecpar, this->codec_ctx_);
    }

    if (avio_open(&(this->format_ctx_->pb), this->output_file_.c_str(), AVIO_FLAG_WRITE) < 0)
    {
      fprintf(stderr, "Could not open output file\n");
      exit(1);
    }

    int result = avformat_write_header(this->format_ctx_, NULL);
  }

  std::cout << "Encoder " << this->encoder_name_<< " initialized." << std::endl;
}

void VideoEncoding::encode_frame_to_file(cv::Mat* frame,
                                          int64_t frame_count) {
  convertBGRAtoNV12(frame);
  
  // Set the PTS based on the frame count and codec time base
  this->frame_nv12->pts = frame_count;

  if (avcodec_send_frame(this->codec_ctx_, this->frame_nv12) < 0)
  {
    fprintf(stderr, "Error sending frame for encoding\n");
  }

  while (avcodec_receive_packet(this->codec_ctx_, this->pkt_) == 0)
  {
    this->pkt_->stream_index = 0;
    this->pkt_->pts = av_rescale_q(this->pkt_->pts, 
                            this->codec_ctx_->time_base, 
                            this->format_ctx_->streams[0]->time_base);
    this->pkt_->dts = av_rescale_q(this->pkt_->dts, 
                            this->codec_ctx_->time_base, 
                            this->format_ctx_->streams[0]->time_base);
    this->pkt_->duration = av_rescale_q(this->pkt_->duration, 
                                  this->codec_ctx_->time_base, 
                                  this->format_ctx_->streams[0]->time_base);

    av_interleaved_write_frame(this->format_ctx_, this->pkt_);
    av_packet_unref(this->pkt_);
  }
}

void VideoEncoding::convertBGRAtoNV12(const cv::Mat* bgra) {
  // Ensure the input format is NV12
  if (this->frame_nv12->format != AV_PIX_FMT_NV12)
  {
    std::cerr << "Invalid format. Expected NV12!" << std::endl;
    return;
  }

  int ret = libyuv::ARGBToNV12(
    bgra->data, bgra->step,                // Input BGRA buffer and its stride
    this->frame_nv12->data[0], this->frame_nv12->linesize[0],  // Y plane and its stride
    this->frame_nv12->data[1], this->frame_nv12->linesize[1],  // Interleaved UV plane and its stride
    bgra->cols, bgra->rows                         // Image dimensions
  );

  if (ret != 0)
  {
    std::cerr << "libyuv ARGBToNV12 failed with error code: " << ret << std::endl;
  }
}

void VideoEncoding::encode_frame_to_stream(cv::Mat* frame, int64_t frame_count) {
  convertBGRAtoNV12(frame);

  this->frame_nv12->pts = frame_count;

  if (avcodec_send_frame(this->codec_ctx_, this->frame_nv12) < 0) 
  {
    fprintf(stderr, "Error sending frame for encoding\n");
  }

  if (avcodec_receive_packet(this->codec_ctx_, this->pkt_) != 0)
  {
    fprintf(stderr, "Error receiving packet\n");
  }

  // Send packet size
  if (send_all(this->socket_, &(this->pkt_->size), sizeof(int)) < 0) {
    fprintf(stderr, "Error sending complete packet size\n");
  }

  // Send packet data
  if (send_all(this->socket_, this->pkt_->data, this->pkt_->size) < 0) {
    fprintf(stderr, "Error sending packet data\n");
  }

  av_packet_unref(this->pkt_);
}

AVCodecContext* VideoEncoding::get_codec_ctx() {
  return this->codec_ctx_;
}

AVFormatContext* VideoEncoding::get_format_ctx() {
  return this->format_ctx_;
}

int VideoEncoding::get_width() {
  return this->width_;
}

int VideoEncoding::get_height() {
  return this->height_;
}
