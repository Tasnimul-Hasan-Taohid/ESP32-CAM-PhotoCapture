#pragma once

// ─────────────────────────────────────────────────────────────────
// ESP32-CAM Photo Capture — config.h
// Supports: AI-Thinker ESP32-CAM (default), TTGO T-Journal,
//           M5Stack ESP32CAM, Wrover Kit
// ─────────────────────────────────────────────────────────────────

// ─── Board Preset ────────────────────────────────────────────────
// Uncomment ONE of the following to match your board:

#define BOARD_AI_THINKER       // Most common — the cheap black one
// #define BOARD_TTGO_T_JOURNAL
// #define BOARD_M5STACK_WIDE
// #define BOARD_WROVER_KIT

// ─── AI-Thinker Pin Map ──────────────────────────────────────────
#ifdef BOARD_AI_THINKER
  #define CAM_PIN_PWDN    32
  #define CAM_PIN_RESET   -1
  #define CAM_PIN_XCLK     0
  #define CAM_PIN_SIOD    26
  #define CAM_PIN_SIOC    27
  #define CAM_PIN_D7      35
  #define CAM_PIN_D6      34
  #define CAM_PIN_D5      39
  #define CAM_PIN_D4      36
  #define CAM_PIN_D3      21
  #define CAM_PIN_D2      19
  #define CAM_PIN_D1      18
  #define CAM_PIN_D0       5
  #define CAM_PIN_VSYNC   25
  #define CAM_PIN_HREF    23
  #define CAM_PIN_PCLK    22
  #define FLASH_LED_PIN    4  // Onboard flash LED
  #define BUTTON_PIN       -1 // No dedicated button on AI-Thinker
                              // Use GPIO0 if you wire one
#endif

// ─── TTGO T-Journal Pin Map ──────────────────────────────────────
#ifdef BOARD_TTGO_T_JOURNAL
  #define CAM_PIN_PWDN     0
  #define CAM_PIN_RESET   15
  #define CAM_PIN_XCLK    27
  #define CAM_PIN_SIOD    25
  #define CAM_PIN_SIOC    23
  #define CAM_PIN_D7      19
  #define CAM_PIN_D6      36
  #define CAM_PIN_D5      18
  #define CAM_PIN_D4       5
  #define CAM_PIN_D3      34
  #define CAM_PIN_D2      35
  #define CAM_PIN_D1      17
  #define CAM_PIN_D0      16
  #define CAM_PIN_VSYNC   22
  #define CAM_PIN_HREF    26
  #define CAM_PIN_PCLK    21
  #define FLASH_LED_PIN   -1
  #define BUTTON_PIN       0
#endif

// ─── M5Stack Wide Pin Map ────────────────────────────────────────
#ifdef BOARD_M5STACK_WIDE
  #define CAM_PIN_PWDN    -1
  #define CAM_PIN_RESET   15
  #define CAM_PIN_XCLK    27
  #define CAM_PIN_SIOD    22
  #define CAM_PIN_SIOC    23
  #define CAM_PIN_D7      19
  #define CAM_PIN_D6      36
  #define CAM_PIN_D5      18
  #define CAM_PIN_D4       5
  #define CAM_PIN_D3      34
  #define CAM_PIN_D2      35
  #define CAM_PIN_D1      17
  #define CAM_PIN_D0      16
  #define CAM_PIN_VSYNC   25
  #define CAM_PIN_HREF    26
  #define CAM_PIN_PCLK    21
  #define FLASH_LED_PIN    2
  #define BUTTON_PIN      38
#endif

// ─── Camera Quality ──────────────────────────────────────────────
// Resolution options (comment/uncomment ONE):
//   FRAMESIZE_QQVGA  160x120
//   FRAMESIZE_QVGA   320x240
//   FRAMESIZE_VGA    640x480  ← default, good balance
//   FRAMESIZE_SVGA   800x600
//   FRAMESIZE_XGA   1024x768
//   FRAMESIZE_SXGA  1280x1024
//   FRAMESIZE_UXGA  1600x1200 ← highest quality, slow

#define PHOTO_RESOLUTION   FRAMESIZE_SVGA   // 800x600 — good default
#define PHOTO_QUALITY      12               // JPEG quality: 0–63, lower = better
#define PHOTO_BRIGHTNESS    0               // -2 to +2
#define PHOTO_CONTRAST      0               // -2 to +2
#define PHOTO_SATURATION    0               // -2 to +2
#define ENABLE_FLASH        true            // Flash LED during capture

// ─── SD Card ─────────────────────────────────────────────────────
#define SD_MOUNT_POINT     "/sdcard"
#define PHOTO_FOLDER       "/photos"        // Folder inside SD card
#define FILENAME_PREFIX    "img_"           // e.g. img_0001.jpg
#define MAX_PHOTOS         9999             // Roll over after this

// ─── WiFi (optional — for web interface) ─────────────────────────
#define ENABLE_WIFI         true
#define WIFI_SSID          "YourSSID"       // ← Change this
#define WIFI_PASSWORD      "YourPassword"   // ← Change this
#define WIFI_TIMEOUT_MS    15000
#define WEB_PORT           80

// ─── Capture Modes ───────────────────────────────────────────────
#define SINGLE_SHOT_MODE    true   // One photo per trigger
#define TIMELAPSE_INTERVAL  30000  // Timelapse interval in ms (30s)
#define TIMELAPSE_ENABLED   false  // Set true for timelapse mode

// ─── Button (optional external button on GPIO0) ──────────────────
#define BTN_DEBOUNCE_MS    50

// ─── Debug ───────────────────────────────────────────────────────
#define DEBUG_BAUD         115200
