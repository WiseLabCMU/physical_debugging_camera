#include <opencv2/opencv.hpp>
#include <jsoncpp/json/json.h>

#include "camera_capture.hpp"
#include "pipe.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <csignal>

PipeDataInCollection<void*>* dImageFrame;
CameraCapture* capture;
std::atomic<bool> capture_running = true;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";

  capture_running = false;

  capture->stop_capture();

  // Put empty images to the pipe to unblock the consumer thread
  for (int i = 0; i < dImageFrame->get_size(); i++)
  {
    dImageFrame->put(i, nullptr);
  }

  // exit(signum);
}

void image_consumer(PipeDataInCollection<void*>* dImageFrame,
                    Json::Value config) {
  int device_count = config["number_cameras"].asInt();
  int camera_width = config["image_width"].asInt();
  int camera_height = config["image_height"].asInt();

  cv::Mat color_img(camera_height, camera_width, CV_8UC3);

  while (capture_running)
  {
    for (size_t idx = 0; idx < device_count; idx++)
    {
      void* rawData = dImageFrame->fetch(idx);
      if (rawData == nullptr)
      {
        std::cout << "Received null data, exiting consumer loop...\n";
        continue;
      }

      color_img.data = (uchar*)rawData;

      cv::imshow("frame", color_img);
      cv::waitKey(1);
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 2)
  {
    std::cerr << "usage: " << argv[0] << " <config-json>\n";
    return 1;
  }

  Json::Value jsonConf;
  {
    std::ifstream fs(argv[1]);
    if (!(fs >> jsonConf))
    {
      std::cerr << "Error reading config\n";
    }
  }

  signal(SIGINT, signalHandler);

  int num_cameras = jsonConf["number_cameras"].asInt();

  dImageFrame = new PipeDataInCollection<void*>(num_cameras);

  capture = new CameraCapture(dImageFrame, jsonConf);

  std::thread capture_thread = std::thread(&CameraCapture::start_capture, capture);

  std::thread consumer_thread = std::thread(&image_consumer, dImageFrame, jsonConf);

  if (consumer_thread.joinable())
  {
    consumer_thread.join();
  }

  return 0;
}
