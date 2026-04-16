# 📷 ESP32-CAM Photo Capture

So you've got one of those tiny ESP32-CAM boards — the cheap black one that costs like $5 and somehow has a camera, WiFi, and a microSD slot all on it. Wild little thing. This project turns it into a proper photo capture device that saves pictures directly to the SD card, with a built-in web UI so you can trigger shots, browse your gallery, and download photos from any device on your network.

No cloud. No app to install. Just point it at a subject, hit capture, and the JPEG lands on your SD card.

---

## What it actually does

- Takes photos and saves them as numbered JPEGs to the SD card (`img_0001.jpg`, `img_0002.jpg`, etc.)
- Serves a small web interface over WiFi so you can trigger captures from your phone or laptop
- Live stream mode so you can see what the camera sees before you shoot
- Photo gallery in the browser — browse, download, or delete individual shots
- Optional physical button trigger if you wire one up
- Optional timelapse mode — set an interval, walk away, let it run
- Serial commands for when you just want to quickly test things without touching the web UI

It picks up where it left off. If you have 47 photos on the card and reboot, the next photo becomes `img_0048.jpg`. No overwriting, no drama.

---

## Hardware you need

| Part | Notes |
|---|---|
| ESP32-CAM (AI-Thinker) | The common cheap black one — most widely tested |
| microSD card | Any card up to 32GB works fine. Format it as FAT32 |
| FTDI USB-to-Serial adapter | For flashing. The board has no USB-C — you need this |
| Jumper wires | A few, for the FTDI connection |
| Push button (optional) | Wire to GPIO0 for a physical shutter button |
| 5V power supply | Minimum 500mA. The camera is a bit power-hungry |

> **A note on the FTDI adapter:** The ESP32-CAM doesn't have built-in USB. You'll need a USB-to-Serial adapter (CP2102, CH340, or FTDI) to flash it. Connect TX→RX, RX→TX, GND→GND, and 5V→5V. Pull GPIO0 to GND before flashing, then release after.

---

## Wiring for flashing

```
FTDI Adapter    →    ESP32-CAM
─────────────────────────────
GND             →    GND
5V              →    5V
TX              →    U0R (GPIO3)
RX              →    U0T (GPIO1)
GND             →    IO0  ← only during upload, disconnect after
```

After flashing, disconnect IO0 from GND and press the reset button.

---

## Project structure

```
ESP32-CAM-PhotoCapture/
├── main.ino          ← Entry point, loop, serial commands, timelapse
├── config.h          ← Everything you need to change is in here
├── camera.h / .cpp   ← Camera init, capture, flash, settings
├── storage.h / .cpp  ← SD card, file naming, space reporting
├── webserver.h / .cpp← WiFi web UI, gallery, stream, download
└── README.md
```

---

## Setup (do this first)

Open `config.h` and change two things:

```cpp
// 1. Make sure your board is selected (AI-Thinker is default)
#define BOARD_AI_THINKER   // ← already set, leave it

// 2. Put your WiFi credentials in here
#define WIFI_SSID      "YourNetworkName"
#define WIFI_PASSWORD  "YourPassword"
```

That's genuinely all you need to do for a basic setup. Everything else has sensible defaults.

---

## Library installation

Install these through Arduino's Library Manager (**Sketch → Include Library → Manage Libraries**):

| Library | Purpose |
|---|---|
| ESP32 Arduino Core | Board support — includes `esp_camera.h` |
| WebServer (built-in) | Web UI |
| SD_MMC (built-in) | SD card via SDMMC peripheral |
| FS (built-in) | File system |

The `esp_camera.h` library comes bundled with the ESP32 Arduino core. If you don't have it, go to **File → Preferences → Additional Board Manager URLs** and add:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then install `esp32` from Board Manager.

---

## Flashing settings

In Arduino IDE, these settings matter:

| Setting | Value |
|---|---|
| Board | AI Thinker ESP32-CAM |
| Upload Speed | 115200 |
| CPU Frequency | 240MHz |
| Flash Frequency | 80MHz |
| Flash Mode | QIO |
| Partition Scheme | Huge APP (3MB No OTA) |

The partition scheme matters — the camera library is large and you'll get a "sketch too big" error with the default setting.

---

## Using it

### Web interface

Once it boots and connects to WiFi, open Serial Monitor and you'll see something like:

```
[WiFi] Connected! IP: 192.168.1.47
[WEB] Server running at http://192.168.1.47
```

Open that address in any browser on the same network. You'll get:

- **Take Photo** — captures and saves to SD card, tells you the filename
- **Live Stream** — MJPEG stream so you can frame your shot
- **Gallery** — grid of saved photos, click to download, ✕ to delete
- **Storage Info** — card capacity, used space, photo count

### Serial commands

With Serial Monitor open at 115200 baud, type single letters:

| Key | Action |
|---|---|
| `c` | Take a photo |
| `l` | List all photos on the SD card |
| `i` | Print status (camera, SD, photo count, free space) |
| `f` | Flash the LED for half a second |
| `e` | Delete ALL photos (asks you to confirm) |
| `h` | Show the command list |

### Physical button

If you wire a momentary button between GPIO0 and GND, it'll trigger a capture on press. The debounce is handled, so a single press = one photo. Set `BUTTON_PIN` in `config.h` to whichever GPIO you use.

### Timelapse mode

In `config.h`:

```cpp
#define TIMELAPSE_ENABLED   true
#define TIMELAPSE_INTERVAL  30000   // 30 seconds between shots
```

Photos go into a numbered subfolder: `/photos/tl_12345/frame_0001.jpg`, etc. The session ID changes each boot so timelapses don't mix together.

---

## Photo quality settings

All in `config.h`. The defaults are a good starting point:

```cpp
#define PHOTO_RESOLUTION   FRAMESIZE_SVGA   // 800x600
#define PHOTO_QUALITY      12               // 0–63, lower = better quality
```

Resolution options, from small to large:

| Setting | Size | Good for |
|---|---|---|
| `FRAMESIZE_QQVGA` | 160×120 | Thumbnails, very fast |
| `FRAMESIZE_QVGA` | 320×240 | Quick preview |
| `FRAMESIZE_VGA` | 640×480 | General use |
| `FRAMESIZE_SVGA` | 800×600 | **Default — good balance** |
| `FRAMESIZE_XGA` | 1024×768 | Higher quality |
| `FRAMESIZE_SXGA` | 1280×1024 | Near-max, needs PSRAM |
| `FRAMESIZE_UXGA` | 1600×1200 | Maximum, needs PSRAM, slow |

If your board has PSRAM (most AI-Thinker boards do), you can use the larger sizes without issues. If you're getting crashes or blank images at high resolution, drop it down one step.

---

## Troubleshooting

**Camera init failed on boot**

This is almost always a pin map issue. Make sure `#define BOARD_AI_THINKER` (or your board) is the only one uncommented in `config.h`. Also check you're powered from 5V at enough current — 3.3V is not enough for the camera.

**SD card not detected**

- Format the card as FAT32 (not exFAT)
- Try a different card — some cheap cards are fussy
- Check the card is pushed in all the way
- Make sure you're using 1-bit SDMMC mode (the default in this code) — the AI-Thinker shares some SD pins with the flash which makes 4-bit mode unreliable

**Photos are dark / overexposed**

Adjust `PHOTO_BRIGHTNESS` in `config.h`. Range is -2 to +2. The OV2640 sensor also benefits from good ambient light — the onboard flash is quite weak, more of a "focus assist" than a real flash.

**WiFi connects but web UI doesn't load**

Make sure your phone/laptop is on the same network. The web server runs on port 80. If something else on your network is blocking it, try changing `WEB_PORT` to 8080.

**Sketch too big error**

Go to Tools → Partition Scheme → Huge APP (3MB No OTA). This is the most common issue people hit.

**Blurry photos**

The OV2640 lens has a fixed focus. The default focus distance is roughly 0.5m–infinity. For close-up shots under 30cm, you may need to physically rotate the lens a tiny bit (it's threaded). Be careful — they snap off if you force it.

---

## Supported boards

| Board | Status | Notes |
|---|---|---|
| AI-Thinker ESP32-CAM | ✅ Primary | The common cheap one |
| TTGO T-Journal | ✅ Supported | Define in config.h |
| M5Stack ESP32CAM | ✅ Supported | Define in config.h |
| Wrover Kit | ✅ Supported | Define in config.h |
| Other ESP32-CAM variants | 🔶 Untested | Pin map may differ |

---

## Files on the SD card

```
/photos/
  img_0001.jpg
  img_0002.jpg
  img_0003.jpg
  ...
  tl_14523/         ← timelapse session
    frame_0001.jpg
    frame_0002.jpg
```

Plug the SD card into your computer and you'll find them exactly where you'd expect. Standard JPEGs, no proprietary format, open with anything.

---

## License

MIT. Do what you want with it.

---

## A few honest notes

The OV2640 camera on these boards is fine for hobby projects but it's not going to blow anyone away in terms of image quality. Low light is rough. The lens is plastic. At $5 a board you're not getting Canon sensor performance.

That said — for timelapse, pet watching, plant monitoring, security-adjacent stuff, maker projects, or just learning how cameras and microcontrollers talk to each other, it absolutely gets the job done. People have built some genuinely impressive things with these boards. This project just tries to give you a clean, reliable foundation to start from.

If you build something cool with it, that's the whole point.
