#pragma once

#include <opencv2/opencv.hpp>
#include <jsoncpp/json/json.h>

#include "pipe.hpp"
#include <m3api/xiApi.h>

#include <memory>
#include <atomic>

class CameraCapture {
public:
  CameraCapture(PipeDataInCollection<void*>* dImageFrame,
                Json::Value config);

  ~CameraCapture();

  void set_camera_param();

  void query_camera_param();

  void start_capture();

  void stop_capture();


private:
  PipeDataInCollection<void*>* dImageFrame_;
  Json::Value config_;

  int num_devices_ = -1;

  std::atomic<bool> keepRunning_ = true;

  int img_width_ = -1;
  int img_height_ = -1;

  HANDLE* hDevice_ = nullptr;
  XI_IMG* image = nullptr;

  XI_RETURN stat = XI_OK;
};
