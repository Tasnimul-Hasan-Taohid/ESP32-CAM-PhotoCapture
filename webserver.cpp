#include "webserver.h"
#include "SD_MMC.h"

PhotoWebServer::PhotoWebServer(Camera* cam, Storage* storage)
  : _server(WEB_PORT), _cam(cam), _storage(storage), _connected(false) {}

bool PhotoWebServer::begin() {
  _connectWiFi();
  if (!_connected) return false;
  _setupRoutes();
  _server.begin();
  Serial.print("[WEB] Server running at http://");
  Serial.println(WiFi.localIP());
  return true;
}

void PhotoWebServer::handle() {
  if (_connected) _server.handleClient();
}

String PhotoWebServer::getIP() {
  return _connected ? WiFi.localIP().toString() : "not connected";
}

bool PhotoWebServer::isConnected() { return _connected; }

// ─── WiFi Connect ────────────────────────────────────────────
void PhotoWebServer::_connectWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println("\n[WiFi] Timeout — running offline");
      return;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("[WiFi] Connected! IP: ");
  Serial.println(WiFi.localIP());
  _connected = true;
}

// ─── Routes ──────────────────────────────────────────────────
void PhotoWebServer::_setupRoutes() {
  _server.on("/",         HTTP_GET,  [this](){ _handleRoot(); });
  _server.on("/capture",  HTTP_GET,  [this](){ _handleCapture(); });
  _server.on("/stream",   HTTP_GET,  [this](){ _handleStream(); });
  _server.on("/gallery",  HTTP_GET,  [this](){ _handleGallery(); });
  _server.on("/download", HTTP_GET,  [this](){ _handleDownload(); });
  _server.on("/delete",   HTTP_GET,  [this](){ _handleDelete(); });
  _server.on("/info",     HTTP_GET,  [this](){ _handleInfo(); });
  _server.onNotFound([this](){ _handleNotFound(); });
}

// ─── Root ────────────────────────────────────────────────────
void PhotoWebServer::_handleRoot() {
  String body = R"html(
    <div class="hero">
      <h1>📷 ESP32-CAM</h1>
      <p class="sub">Photo Capture Station</p>
    </div>
    <div class="grid">
      <a class="card" href="/capture">
        <span class="icon">📸</span>
        <span class="label">Take Photo</span>
        <span class="desc">Capture and save to SD card</span>
      </a>
      <a class="card" href="/stream">
        <span class="icon">🎥</span>
        <span class="label">Live Stream</span>
        <span class="desc">View live camera feed</span>
      </a>
      <a class="card" href="/gallery">
        <span class="icon">🖼️</span>
        <span class="label">Gallery</span>
        <span class="desc">Browse saved photos</span>
      </a>
      <a class="card" href="/info">
        <span class="icon">💾</span>
        <span class="label">Storage Info</span>
        <span class="desc">SD card status and space</span>
      </a>
    </div>
  )html";
  _server.send(200, "text/html", _buildPage("ESP32-CAM Home", body));
}

// ─── Capture ─────────────────────────────────────────────────
void PhotoWebServer::_handleCapture() {
  if (!_cam->isReady()) {
    _server.send(500, "text/html",
      _buildPage("Error", "<p class='err'>Camera not ready.</p>"));
    return;
  }

  _cam->flashOn();
  delay(80);
  camera_fb_t* fb = _cam->capture();
  _cam->flashOff();

  if (!fb) {
    _server.send(500, "text/html",
      _buildPage("Error", "<p class='err'>Capture failed.</p>"));
    return;
  }

  String filename = _storage->savePhoto(fb->buf, fb->len);
  _cam->releaseFrame(fb);

  if (filename.isEmpty()) {
    _server.send(500, "text/html",
      _buildPage("Error", "<p class='err'>Failed to save to SD card.</p>"));
    return;
  }

  String body = "<div class='result'>"
    "<p class='ok'>✅ Photo saved!</p>"
    "<p class='filename'>" + filename + "</p>"
    "<p class='count'>Total: " + String(_storage->getPhotoCount()) + " photos on card</p>"
    "<div class='btns'>"
    "<a class='btn' href='/gallery'>View Gallery</a>"
    "<a class='btn' href='/'>Home</a>"
    "<a class='btn-primary' href='/capture'>Take Another</a>"
    "</div></div>";

  _server.send(200, "text/html", _buildPage("Photo Saved", body));
}

// ─── Stream (MJPEG) ──────────────────────────────────────────
void PhotoWebServer::_handleStream() {
  WiFiClient client = _server.client();

  String header = "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "Access-Control-Allow-Origin: *\r\n\r\n";
  client.print(header);

  Serial.println("[WEB] Streaming started");

  while (client.connected()) {
    camera_fb_t* fb = _cam->capture();
    if (!fb) { delay(100); continue; }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\n"
                  "Content-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    _cam->releaseFrame(fb);

    delay(50); // ~20fps max
    yield();
  }
  Serial.println("[WEB] Stream ended");
}

// ─── Gallery ─────────────────────────────────────────────────
void PhotoWebServer::_handleGallery() {
  File dir = SD_MMC.open(PHOTO_FOLDER);
  if (!dir || !dir.isDirectory()) {
    _server.send(200, "text/html",
      _buildPage("Gallery", "<p>No photos yet. <a href='/capture'>Take one!</a></p>"));
    return;
  }

  String items = "<div class='gallery-header'>"
    "<h2>📷 Photo Gallery</h2>"
    "<p>" + String(_storage->getPhotoCount()) + " photos • " +
    _storage->formatSize(_storage->getFreeSpaceBytes()) + " free</p></div>"
    "<div class='gallery'>";

  File entry = dir.openNextFile();
  int count = 0;
  while (entry && count < 50) {
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      if (name.endsWith(".jpg") || name.endsWith(".jpeg")) {
        items += "<div class='thumb'>"
          "<a href='/download?f=" + name + "'>"
          "<div class='thumb-placeholder'>📷</div>"
          "<span class='thumb-name'>" + name + "</span>"
          "<span class='thumb-size'>" + _storage->formatSize(entry.size()) + "</span>"
          "</a>"
          "<a class='del-btn' href='/delete?f=" + name + "' "
          "onclick=\"return confirm('Delete " + name + "?')\">✕</a>"
          "</div>";
        count++;
      }
    }
    entry = dir.openNextFile();
  }
  items += "</div>";

  if (count == 0) {
    items = "<p>No photos yet. <a href='/capture'>Take one!</a></p>";
  }

  _server.send(200, "text/html", _buildPage("Gallery", items));
}

// ─── Download ────────────────────────────────────────────────
void PhotoWebServer::_handleDownload() {
  if (!_server.hasArg("f")) {
    _server.send(400, "text/plain", "Missing filename");
    return;
  }

  String filename = _server.arg("f");
  String path     = String(PHOTO_FOLDER) + "/" + filename;
  File   file     = SD_MMC.open(path, FILE_READ);

  if (!file) {
    _server.send(404, "text/plain", "File not found: " + filename);
    return;
  }

  _server.sendHeader("Content-Disposition",
    "attachment; filename=\"" + filename + "\"");
  _server.sendHeader("Content-Length", String(file.size()));
  _server.streamFile(file, "image/jpeg");
  file.close();
}

// ─── Delete ──────────────────────────────────────────────────
void PhotoWebServer::_handleDelete() {
  if (!_server.hasArg("f")) {
    _server.send(400, "text/plain", "Missing filename");
    return;
  }

  String filename = _server.arg("f");
  bool   ok       = _storage->deletePhoto(filename);

  String body = ok
    ? "<p class='ok'>✅ Deleted: " + filename + "</p><a class='btn' href='/gallery'>Back to Gallery</a>"
    : "<p class='err'>❌ Failed to delete: " + filename + "</p><a class='btn' href='/gallery'>Back</a>";

  _server.send(200, "text/html", _buildPage("Delete", body));
}

// ─── Info ────────────────────────────────────────────────────
void PhotoWebServer::_handleInfo() {
  uint64_t total = _storage->getTotalSpaceBytes();
  uint64_t used  = _storage->getUsedSpaceBytes();
  uint64_t free_ = _storage->getFreeSpaceBytes();
  float    pct   = total > 0 ? ((float)used / total * 100.0f) : 0;

  String body = "<h2>💾 Storage Info</h2>"
    "<table class='info-table'>"
    "<tr><td>Photos taken</td><td><b>" + String(_storage->getPhotoCount()) + "</b></td></tr>"
    "<tr><td>Total space</td><td><b>" + _storage->formatSize(total) + "</b></td></tr>"
    "<tr><td>Used</td><td><b>" + _storage->formatSize(used) + "</b></td></tr>"
    "<tr><td>Free</td><td><b>" + _storage->formatSize(free_) + "</b></td></tr>"
    "<tr><td>Used %</td><td><b>" + String(pct, 1) + "%</b></td></tr>"
    "</table>"
    "<div class='bar-wrap'><div class='bar-fill' style='width:" + String(pct, 0) + "%'></div></div>"
    "<p class='note'>Resolution: " + _cam->resolutionLabel() + "</p>"
    "<a class='btn' href='/'>Home</a>";

  _server.send(200, "text/html", _buildPage("Storage Info", body));
}

void PhotoWebServer::_handleNotFound() {
  _server.send(404, "text/html",
    _buildPage("404", "<p>Page not found. <a href='/'>Go home</a></p>"));
}

// ─── HTML builder ────────────────────────────────────────────
String PhotoWebServer::_buildPage(const String& title, const String& body) {
  return R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>)html" + title + R"html(</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
    background:#0f0f0f;color:#eee;min-height:100vh;padding:20px}
  .hero{text-align:center;padding:30px 0 10px}
  .hero h1{font-size:2rem;color:#fff}
  .sub{color:#888;margin-top:4px}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));
    gap:16px;max-width:700px;margin:24px auto}
  .card{display:flex;flex-direction:column;align-items:center;
    background:#1a1a1a;border:1px solid #333;border-radius:12px;
    padding:24px 16px;text-decoration:none;color:#eee;
    transition:all 0.15s}
  .card:hover{border-color:#ff6a00;background:#1e1a16}
  .icon{font-size:2rem;margin-bottom:8px}
  .label{font-weight:700;font-size:0.95rem}
  .desc{font-size:0.75rem;color:#888;margin-top:4px;text-align:center}
  h2{margin-bottom:16px;font-size:1.2rem}
  .result,.info-table-wrap{max-width:500px;margin:20px auto;text-align:center}
  .ok{color:#00e5a0;font-size:1.1rem;margin-bottom:8px}
  .err{color:#ff4060;font-size:1.1rem}
  .filename{font-family:monospace;color:#ff9a40;margin:8px 0}
  .count{color:#888;margin-bottom:16px}
  .btns{display:flex;gap:10px;justify-content:center;flex-wrap:wrap;margin-top:16px}
  .btn{background:#1a1a1a;border:1px solid #444;color:#eee;
    padding:8px 18px;border-radius:8px;text-decoration:none;font-size:0.85rem}
  .btn:hover{border-color:#ff6a00}
  .btn-primary{background:#ff6a00;border-color:#ff6a00;color:#fff;
    padding:8px 18px;border-radius:8px;text-decoration:none;font-size:0.85rem}
  .gallery-header{text-align:center;margin-bottom:16px}
  .gallery-header p{color:#888;font-size:0.85rem;margin-top:4px}
  .gallery{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));
    gap:12px;max-width:900px;margin:0 auto}
  .thumb{background:#1a1a1a;border:1px solid #333;border-radius:8px;
    overflow:hidden;position:relative}
  .thumb a{display:block;text-decoration:none;color:#eee;padding:10px}
  .thumb-placeholder{font-size:2.5rem;text-align:center;padding:10px 0}
  .thumb-name{display:block;font-size:0.72rem;font-family:monospace;
    color:#ff9a40;word-break:break-all}
  .thumb-size{display:block;font-size:0.68rem;color:#666;margin-top:2px}
  .del-btn{position:absolute;top:6px;right:6px;background:rgba(255,64,96,0.8);
    color:#fff;border-radius:50%;width:20px;height:20px;
    display:flex;align-items:center;justify-content:center;
    text-decoration:none;font-size:0.7rem;line-height:1}
  .del-btn:hover{background:#ff4060}
  .info-table{width:100%;max-width:400px;border-collapse:collapse;margin:16px auto}
  .info-table td{padding:10px 14px;border-bottom:1px solid #2a2a2a;font-size:0.9rem}
  .info-table td:first-child{color:#888}
  .bar-wrap{max-width:400px;margin:16px auto;height:10px;
    background:#2a2a2a;border-radius:5px;overflow:hidden}
  .bar-fill{height:100%;background:linear-gradient(90deg,#ff6a00,#ff9a40);
    border-radius:5px;transition:width 0.3s}
  .note{color:#888;font-size:0.8rem;margin:12px 0 20px;text-align:center}
  nav{max-width:900px;margin:0 auto 20px;display:flex;gap:12px}
  nav a{color:#888;text-decoration:none;font-size:0.85rem}
  nav a:hover{color:#ff9a40}
</style>
</head>
<body>
<nav><a href="/">Home</a><a href="/gallery">Gallery</a>
<a href="/capture">Capture</a><a href="/info">Storage</a></nav>
)html" + body + R"html(
</body></html>)html";
}
