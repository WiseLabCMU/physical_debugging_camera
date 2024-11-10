#include "camera_capture.hpp"

#include <opencv2/opencv.hpp>
#include <jsoncpp/json/json.h>

#include <m3api/xiApi.h>

#include <memory>
#include <iostream>

#define HandleResult(res,place) if (res!=XI_OK) {printf("Error after %s (%d)\n",place,res);}

CameraCapture::CameraCapture(PipeDataInCollection<void*>* dImageFrame,
                              Json::Value config) {
  this->dImageFrame_ = dImageFrame;
  this->config_ = config;
  this->num_devices_ = this->config_["number_cameras"].asInt();

  this->img_width_ = this->config_["image_width"].asInt();
  this->img_height_ = this->config_["image_height"].asInt();

  DWORD *pNumberDevices = new DWORD;
  this->stat = xiGetNumberDevices(pNumberDevices);
  std::cout << "Number of devices: " << *pNumberDevices << std::endl;

  if (*pNumberDevices == 0) 
  {
    std::cerr << "No cameras found\n";
    return;
  }

  this->hDevice_= new HANDLE;
  this->stat = xiOpenDevice(0, this->hDevice_);
  HandleResult(this->stat,"xiOpenDevice");

  this->image = new XI_IMG;

  this->set_camera_param();

  this->query_camera_param();
}

CameraCapture::~CameraCapture() {
  
}

void CameraCapture::set_camera_param() {
  // this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_EXPOSURE, 10000);
  // HandleResult(this->stat,"xiSetParam (exposure set)");

  // Sensor configuration
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_IMAGE_DATA_FORMAT, XI_RGB24);
  HandleResult(this->stat,"xiSetParam (XI_PRM_IMAGE_DATA_FORMAT set)");
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_AUTO_WB, XI_ON);
  HandleResult(this->stat,"xiSetParam (XI_PRM_AUTO_WB set)");
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_AEAG, XI_ON);
  HandleResult(this->stat,"xiSetParam (XI_PRM_AEAG set)");

  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_AE_MAX_LIMIT, 200000);
  HandleResult(this->stat,"xiSetParam (XI_PRM_AE_MAX_LIMIT set)");
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_AG_MAX_LIMIT, 20);
  HandleResult(this->stat,"xiSetParam (XI_PRM_AG_MAX_LIMIT set)");
  

  // Image configuration
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_WIDTH, this->img_width_);
  HandleResult(this->stat,"xiSetParam (XI_PRM_WIDTH set)");
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_HEIGHT, this->img_height_);
  HandleResult(this->stat,"xiSetParam (XI_PRM_HEIGHT set)");

  // High frame rate configuration
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_SENSOR_FEATURE_SELECTOR, XI_SENSOR_FEATURE_ZEROROT_ENABLE);
  HandleResult(this->stat,"xiSetParam (XI_PRM_SENSOR_FEATURE_SELECTOR set)");
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_SENSOR_FEATURE_VALUE, XI_ON);
  HandleResult(this->stat,"xiSetParam (XI_PRM_SENSOR_FEATURE_VALUE set)");
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_DOWNSAMPLING, XI_DWN_2x2);
  HandleResult(this->stat,"xiSetParam (XI_PRM_DOWNSAMPLING set)");
  this->stat = xiSetParamInt(*this->hDevice_, XI_PRM_DOWNSAMPLING_TYPE, XI_SKIPPING);
  HandleResult(this->stat,"xiSetParam (XI_PRM_DOWNSAMPLING_TYPE set)");
}

void CameraCapture::query_camera_param() {
  int query_result = -1;

  this->stat = xiGetParamInt(*this->hDevice_, XI_PRM_EXPOSURE, &query_result);
  std::cout << "XI_PRM_EXPOSURE: " << query_result << std::endl;

  this->stat = xiGetParamInt(*this->hDevice_, XI_PRM_AUTO_WB, &query_result);
  std::cout << "XI_PRM_AUTO_WB: " << query_result << std::endl;

  this->stat = xiGetParamInt(*this->hDevice_, XI_PRM_AEAG, &query_result);
  std::cout << "XI_PRM_AEAG: " << query_result << std::endl;

  this->stat = xiGetParamInt(*this->hDevice_, XI_PRM_AE_MAX_LIMIT, &query_result);
  std::cout << "XI_PRM_AE_MAX_LIMIT: " << query_result << std::endl;

  this->stat = xiGetParamInt(*this->hDevice_, XI_PRM_AG_MAX_LIMIT, &query_result);
  std::cout << "XI_PRM_AG_MAX_LIMIT: " << query_result << std::endl;
}

void CameraCapture::start_capture() {
  this->stat = xiStartAcquisition(*this->hDevice_);
  HandleResult(this->stat, "xiStartAcquisition");

  auto lastTime = std::chrono::high_resolution_clock::now();
  int frameCount = 0;
  double fps = 0.0;
  
  while (this->keepRunning_)
  {
    this->stat = xiGetImage(*this->hDevice_, 5000, this->image);
    HandleResult(stat, "xiGetImage");

    // std::cout << "Image resolution: " << this->image->width << 
      // "x" << this->image->height << std::endl;

    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = currentTime - lastTime;

    for (size_t idx = 0; idx < this->num_devices_; idx++)
    {
      this->dImageFrame_->put(idx, (void*)this->image->bp);
    }

    // Log every second
    frameCount++;
    if (elapsed.count() >= 1.0) 
    {
      fps = frameCount / elapsed.count();
      frameCount = 0;
      lastTime = currentTime;

      std::cout << "FPS: " << fps << std::endl;
    }
  }
}

void CameraCapture::stop_capture() {
  this->keepRunning_ = false;

  this->stat = xiStopAcquisition(*this->hDevice_);
  HandleResult(this->stat,"xiStopAcquisition");

  this->stat = xiCloseDevice(*this->hDevice_);
  HandleResult(this->stat,"xiCloseDevice");
}
