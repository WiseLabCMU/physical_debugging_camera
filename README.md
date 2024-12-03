# physical_debugging_camera

## Hardware Requirement
NVIDIA GPU is required for fast video coding.

If you do not have access to a CUDA-capable GPU, it is possible to modify the video coding support to use software codec, e.g., x265). Although video coding performance will be significantly worse.

## Supported OS
Ubuntu 22.04 (x86_64)

## Dependencies
[XIMEA Camera SDK](https://www.ximea.com/support/wiki/apis/XIMEA_Linux_Software_Package)

### Install with apt
OpenCV, JsonCpp
```
sudo apt-get update
sudo apt install libopencv-dev libjsoncpp-dev
```

### Build from source
[FFMPEG](https://docs.nvidia.com/video-technologies/video-codec-sdk/12.0/ffmpeg-with-nvidia-gpu/index.html) (Compile from source to get the latest AV1 codec support on NVIDIA RTX 4000 Series GPU)

[libyuv](https://chromium.googlesource.com/libyuv/libyuv/)

## Build
```
mkdir build
cd build
cmake ..
make -j
```

## Capturing camera data
```
./build/app/camera_stream ../camera_config.json
```

## View saved video file
Under the root project directory, you'll find saved video `output_0.mp4`, and associated per-frame epoch timestamp `output_timestamps_session_0.txt`

The output filename can be changed in the `camera_config.json`.
