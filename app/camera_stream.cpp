#include <opencv2/opencv.hpp>
#include <jsoncpp/json/json.h>

#include "camera_capture.hpp"
#include "pipe.hpp"
#include "video_encoding.hpp"

#include <chrono>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <csignal>
#include <cstdlib>

PipeDataInCollection<void*>* dImageFrame;
CameraCapture* capture;
std::atomic<bool> capture_running = true;

void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";

  capture_running = false;

  if (capture) {
    capture->stop_capture();
  }

  // Put empty images to the pipe to unblock the consumer thread
  for (int i = 0; i < dImageFrame->get_size(); i++)
  {
    dImageFrame->put(i, nullptr);
  }
}

void image_consumer(PipeDataInCollection<void*>* dImageFrame,
                    Json::Value config) {
  int device_count = config["number_cameras"].asInt();
  int camera_width = config["image_width"].asInt();
  int camera_height = config["image_height"].asInt();

  Json::Value jsonVideoConf = config["video_encoding"];
  VideoEncoding* video_encoder = new VideoEncoding(jsonVideoConf, "output", 0, -1);

  cv::Mat color_img(camera_height, camera_width, CV_8UC3);
  cv::Mat bgra_img(camera_height, camera_width, CV_8UC4);

  int frame_count = 0;
  std::ofstream timestamp_log(jsonVideoConf["output_timestamp_path"].asString() + \
                              "_session_" + std::to_string(0) + ".txt");

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

      // Sampling timestamp
      auto now = std::chrono::system_clock::now();
      auto epoch_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

      timestamp_log << "Frame " << frame_count << " timestamp: " 
                  << epoch_time << "\n";


      color_img.data = (uchar*)rawData;
      cv::cvtColor(color_img, bgra_img, cv::COLOR_BGR2BGRA);

      video_encoder->encode_frame_to_file(&bgra_img, frame_count);
      frame_count++;

      // Flush to ensure the data is written to the file after each frame
      timestamp_log.flush();

      // cv::imshow("frame", color_img);
      // cv::waitKey(1);
    }
  }
  av_write_trailer(video_encoder->get_format_ctx());

  timestamp_log.close();

  std::cout << "Video session " << "0" << " finished encoding.\n";

  delete video_encoder;
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

  if (capture_thread.joinable())
  {
    capture_thread.join();
  }

  if (consumer_thread.joinable())
  {
    consumer_thread.join();
  }

  delete dImageFrame;
  delete capture;

  return 0;
}
