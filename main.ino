#include "config.h"
#include "camera.h"
#include "storage.h"

#if ENABLE_WIFI
  #include "webserver.h"
#endif

// ─── Objects ───────────────────────────────────────────────────
Camera  cam;
Storage storage;

#if ENABLE_WIFI
  PhotoWebServer* webServer = nullptr;
#endif

// ─── Button state (optional physical button) ───────────────────
#if defined(BUTTON_PIN) && BUTTON_PIN >= 0
  bool     lastBtnState    = HIGH;
  bool     btnPressed      = false;
  unsigned long btnDebounce = 0;
#endif

// ─── Timelapse state ───────────────────────────────────────────
#if TIMELAPSE_ENABLED
  int           tlSession  = 0;
  unsigned long lastTlShot = 0;
#endif

// ─── Helpers ───────────────────────────────────────────────────
bool takeAndSave() {
  Serial.println("[APP] Taking photo...");

  cam.flashOn();
  delay(80);
  camera_fb_t* fb = cam.capture();
  cam.flashOff();

  if (!fb) {
    Serial.println("[APP] Capture failed");
    return false;
  }

  String name = storage.savePhoto(fb->buf, fb->len);
  cam.releaseFrame(fb);

  if (name.isEmpty()) {
    Serial.println("[APP] Save failed");
    return false;
  }

  Serial.print("[APP] Saved as: ");
  Serial.println(name);
  Serial.print("[APP] Total photos: ");
  Serial.println(storage.getPhotoCount());
  Serial.print("[APP] Free space:   ");
  Serial.println(storage.formatSize(storage.getFreeSpaceBytes()));
  return true;
}

void printStatus() {
  Serial.println("─────────────────────────");
  Serial.println("  ESP32-CAM Photo Capture");
  Serial.println("─────────────────────────");
  Serial.print("Camera:   ");
  Serial.println(cam.isReady()    ? "OK" : "FAILED");
  Serial.print("SD Card:  ");
  Serial.println(storage.isReady() ? "OK" : "FAILED");
  Serial.print("Photos:   ");
  Serial.println(storage.getPhotoCount());
  Serial.print("Free:     ");
  Serial.println(storage.formatSize(storage.getFreeSpaceBytes()));
  Serial.print("Resolution: ");
  Serial.println(cam.resolutionLabel());
#if ENABLE_WIFI && defined(webServer)
  if (webServer && webServer->isConnected()) {
    Serial.print("Web UI:   http://");
    Serial.println(webServer->getIP());
  }
#endif
#if TIMELAPSE_ENABLED
  Serial.print("Timelapse: every ");
  Serial.print(TIMELAPSE_INTERVAL / 1000);
  Serial.println("s");
#endif
  Serial.println("─────────────────────────");
#if defined(BUTTON_PIN) && BUTTON_PIN >= 0
  Serial.println("Press button to take photo");
#else
  Serial.println("Send 'c' over Serial to capture");
  Serial.println("Send 'l' to list photos");
  Serial.println("Send 'i' for storage info");
#endif
}

// ──────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(DEBUG_BAUD);
  delay(500);
  Serial.println("\n\nESP32-CAM Photo Capture — booting...");

  // Init camera
  if (!cam.begin()) {
    Serial.println("[FATAL] Camera init failed — check wiring and board preset in config.h");
    // Blink built-in LED to signal error
    pinMode(33, OUTPUT);
    while (true) {
      digitalWrite(33, HIGH); delay(200);
      digitalWrite(33, LOW);  delay(200);
    }
  }

  // Warm up camera — discard first few frames
  Serial.println("[APP] Warming up camera...");
  for (int i = 0; i < 3; i++) {
    camera_fb_t* fb = cam.capture();
    if (fb) cam.releaseFrame(fb);
    delay(100);
  }

  // Init SD card
  if (!storage.begin()) {
    Serial.println("[WARN] SD card not available — photos will not be saved");
  } else {
    storage.listPhotos();
  }

  // Button setup
#if defined(BUTTON_PIN) && BUTTON_PIN >= 0
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("[BTN] Button ready on GPIO " + String(BUTTON_PIN));
#endif

  // Timelapse
#if TIMELAPSE_ENABLED
  tlSession = (int)(millis() % 10000);
  lastTlShot = millis();
  Serial.print("[TL] Timelapse session: ");
  Serial.println(tlSession);
#endif

  // Web server
#if ENABLE_WIFI
  webServer = new PhotoWebServer(&cam, &storage);
  if (!webServer->begin()) {
    Serial.println("[WARN] Web server not started (no WiFi)");
  }
#endif

  printStatus();
}

// ──────────────────────────────────────────────────────────────
void loop() {

  // ── Serial commands ────────────────────────────────────────
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'c': case 'C':
        takeAndSave();
        break;
      case 'l': case 'L':
        storage.listPhotos();
        break;
      case 'i': case 'I':
        printStatus();
        break;
      case 'f': case 'F':
        cam.flashPulse(500);
        break;
      case 'e': case 'E':
        Serial.println("Deleting ALL photos...");
        storage.deleteAll();
        break;
      case '?': case 'h': case 'H':
        Serial.println("Commands: c=capture  l=list  i=info  f=flash  e=erase all  h=help");
        break;
    }
  }

  // ── Physical button ────────────────────────────────────────
#if defined(BUTTON_PIN) && BUTTON_PIN >= 0
  bool raw = digitalRead(BUTTON_PIN);
  unsigned long now = millis();
  if (raw != lastBtnState && (now - btnDebounce) > BTN_DEBOUNCE_MS) {
    btnDebounce = now;
    if (raw == LOW && lastBtnState == HIGH) {
      // Button pressed
      takeAndSave();
    }
    lastBtnState = raw;
  }
#endif

  // ── Timelapse ──────────────────────────────────────────────
#if TIMELAPSE_ENABLED
  if (millis() - lastTlShot >= TIMELAPSE_INTERVAL) {
    lastTlShot = millis();
    Serial.println("[TL] Timelapse frame...");
    cam.flashOn();
    delay(50);
    camera_fb_t* fb = cam.capture();
    cam.flashOff();
    if (fb) {
      storage.saveTimelapse(fb->buf, fb->len, tlSession);
      cam.releaseFrame(fb);
    }
  }
#endif

  // ── Web server handler ─────────────────────────────────────
#if ENABLE_WIFI
  if (webServer) webServer->handle();
#endif

  yield();
}
