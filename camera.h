#pragma once

#include <Arduino.h>
#include "esp_camera.h"
#include "config.h"

class Camera {
public:
  Camera();

  bool     begin();
  void     end();

  // Capture a single frame into a buffer.
  // Caller is responsible for freeing with releaseFrame()
  camera_fb_t* capture();
  void         releaseFrame(camera_fb_t* fb);

  // Flash
  void     flashOn();
  void     flashOff();
  void     flashPulse(int ms = 100);

  // Settings (apply after begin())
  bool     setResolution(framesize_t size);
  bool     setQuality(int quality);      // 0–63, lower = better file
  bool     setBrightness(int val);       // -2 to +2
  bool     setContrast(int val);
  bool     setSaturation(int val);
  bool     setFlipVertical(bool flip);
  bool     setFlipHorizontal(bool flip);

  bool     isReady();
  String   resolutionLabel();

private:
  bool     _ready;
  bool     _flashAvailable;

  camera_config_t _buildConfig();
};
