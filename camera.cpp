#include "camera.h"

Camera::Camera() : _ready(false), _flashAvailable(false) {}

// ─── Build camera_config_t from config.h ─────────────────────
camera_config_t Camera::_buildConfig() {
  camera_config_t cfg;

  cfg.pin_pwdn     = CAM_PIN_PWDN;
  cfg.pin_reset    = CAM_PIN_RESET;
  cfg.pin_xclk     = CAM_PIN_XCLK;
  cfg.pin_sscb_sda = CAM_PIN_SIOD;
  cfg.pin_sscb_scl = CAM_PIN_SIOC;
  cfg.pin_d7       = CAM_PIN_D7;
  cfg.pin_d6       = CAM_PIN_D6;
  cfg.pin_d5       = CAM_PIN_D5;
  cfg.pin_d4       = CAM_PIN_D4;
  cfg.pin_d3       = CAM_PIN_D3;
  cfg.pin_d2       = CAM_PIN_D2;
  cfg.pin_d1       = CAM_PIN_D1;
  cfg.pin_d0       = CAM_PIN_D0;
  cfg.pin_vsync    = CAM_PIN_VSYNC;
  cfg.pin_href     = CAM_PIN_HREF;
  cfg.pin_pclk     = CAM_PIN_PCLK;

  cfg.xclk_freq_hz = 20000000;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.pixel_format = PIXFORMAT_JPEG;

  // Use PSRAM if available for larger frames
  if (psramFound()) {
    cfg.frame_size    = PHOTO_RESOLUTION;
    cfg.jpeg_quality  = PHOTO_QUALITY;
    cfg.fb_count      = 2;            // Double buffer for speed
  } else {
    // No PSRAM — fall back to smaller frame
    cfg.frame_size    = FRAMESIZE_SVGA;
    cfg.jpeg_quality  = 12;
    cfg.fb_count      = 1;
    Serial.println("[CAM] No PSRAM — using SVGA fallback");
  }

  cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  return cfg;
}

// ─── Init ────────────────────────────────────────────────────
bool Camera::begin() {
  Serial.println("[CAM] Initialising...");

  camera_config_t cfg = _buildConfig();
  esp_err_t err = esp_camera_init(&cfg);

  if (err != ESP_OK) {
    Serial.printf("[CAM] Init failed: 0x%x\n", err);
    _ready = false;
    return false;
  }

  // Apply initial settings
  sensor_t* s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, PHOTO_BRIGHTNESS);
    s->set_contrast(s,   PHOTO_CONTRAST);
    s->set_saturation(s, PHOTO_SATURATION);
    // OV2640 specific — improve quality
    if (s->id.PID == OV2640_PID) {
      s->set_vflip(s, 0);
      s->set_hmirror(s, 0);
    }
  }

  // Flash LED setup
#if defined(FLASH_LED_PIN) && FLASH_LED_PIN >= 0
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  _flashAvailable = true;
  Serial.println("[CAM] Flash LED ready");
#endif

  _ready = true;
  Serial.println("[CAM] Ready");
  Serial.print("[CAM] Resolution: ");
  Serial.println(resolutionLabel());
  return true;
}

void Camera::end() {
  esp_camera_deinit();
  _ready = false;
}

// ─── Capture ─────────────────────────────────────────────────
camera_fb_t* Camera::capture() {
  if (!_ready) return nullptr;

  // Discard stale frame if double-buffering
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[CAM] Capture failed — null frame");
    return nullptr;
  }

  // Verify JPEG
  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("[CAM] Unexpected format (not JPEG)");
    esp_camera_fb_return(fb);
    return nullptr;
  }

  Serial.printf("[CAM] Captured: %d bytes\n", fb->len);
  return fb;
}

void Camera::releaseFrame(camera_fb_t* fb) {
  if (fb) esp_camera_fb_return(fb);
}

// ─── Flash ───────────────────────────────────────────────────
void Camera::flashOn() {
  if (_flashAvailable && ENABLE_FLASH)
    digitalWrite(FLASH_LED_PIN, HIGH);
}

void Camera::flashOff() {
  if (_flashAvailable)
    digitalWrite(FLASH_LED_PIN, LOW);
}

void Camera::flashPulse(int ms) {
  flashOn();
  delay(ms);
  flashOff();
}

// ─── Settings ────────────────────────────────────────────────
bool Camera::setResolution(framesize_t size) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_framesize(s, size);
  return true;
}

bool Camera::setQuality(int q) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_quality(s, q);
  return true;
}

bool Camera::setBrightness(int val) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_brightness(s, val);
  return true;
}

bool Camera::setContrast(int val) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_contrast(s, val);
  return true;
}

bool Camera::setSaturation(int val) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_saturation(s, val);
  return true;
}

bool Camera::setFlipVertical(bool flip) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_vflip(s, flip ? 1 : 0);
  return true;
}

bool Camera::setFlipHorizontal(bool flip) {
  sensor_t* s = esp_camera_sensor_get();
  if (!s) return false;
  s->set_hmirror(s, flip ? 1 : 0);
  return true;
}

bool Camera::isReady() { return _ready; }

String Camera::resolutionLabel() {
  switch (PHOTO_RESOLUTION) {
    case FRAMESIZE_QQVGA: return "160x120";
    case FRAMESIZE_QVGA:  return "320x240";
    case FRAMESIZE_VGA:   return "640x480";
    case FRAMESIZE_SVGA:  return "800x600";
    case FRAMESIZE_XGA:   return "1024x768";
    case FRAMESIZE_SXGA:  return "1280x1024";
    case FRAMESIZE_UXGA:  return "1600x1200";
    default:              return "unknown";
  }
}
