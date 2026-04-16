#include "storage.h"

Storage::Storage() : _ready(false), _photoIndex(0) {}

// ─── Init ────────────────────────────────────────────────────
bool Storage::begin() {
  Serial.println("[SD] Mounting SD card...");

  // SD_MMC uses the SDMMC peripheral (1-bit mode for AI-Thinker)
  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    Serial.println("[SD] Mount failed. Is a card inserted?");
    _ready = false;
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[SD] No card detected");
    _ready = false;
    return false;
  }

  const char* types[] = {"NONE","MMC","SD","SDHC","UNKNOWN"};
  int typeIdx = min((int)cardType, 4);
  Serial.print("[SD] Card type: ");
  Serial.println(types[typeIdx]);
  Serial.print("[SD] Total space: ");
  Serial.println(formatSize(SD_MMC.totalBytes()));
  Serial.print("[SD] Used space:  ");
  Serial.println(formatSize(SD_MMC.usedBytes()));

  // Create photos folder if missing
  _ensureFolder(PHOTO_FOLDER);

  // Find the next available index
  _photoIndex = _scanHighestIndex() + 1;
  Serial.print("[SD] Next photo index: ");
  Serial.println(_photoIndex);

  _ready = true;
  Serial.println("[SD] Ready");
  return true;
}

void Storage::end() {
  SD_MMC.end();
  _ready = false;
}

// ─── Save photo ──────────────────────────────────────────────
String Storage::savePhoto(const uint8_t* data, size_t len) {
  if (!_ready) {
    Serial.println("[SD] Not ready");
    return "";
  }
  if (!data || len == 0) {
    Serial.println("[SD] Empty frame, skipping");
    return "";
  }

  String filename = _buildFilename(_photoIndex);
  String fullPath = String(PHOTO_FOLDER) + "/" + filename;

  File file = SD_MMC.open(fullPath, FILE_WRITE);
  if (!file) {
    Serial.print("[SD] Failed to open: ");
    Serial.println(fullPath);
    return "";
  }

  size_t written = file.write(data, len);
  file.close();

  if (written != len) {
    Serial.printf("[SD] Write mismatch: %d/%d bytes\n", written, len);
    return "";
  }

  Serial.printf("[SD] Saved: %s (%s)\n", fullPath.c_str(), formatSize(len).c_str());
  _photoIndex++;
  if (_photoIndex > MAX_PHOTOS) _photoIndex = 1;

  return filename;
}

// ─── Save timelapse frame ────────────────────────────────────
String Storage::saveTimelapse(const uint8_t* data, size_t len, int sessionId) {
  if (!_ready || !data || len == 0) return "";

  // Create session subfolder
  String sessionFolder = String(PHOTO_FOLDER) + "/tl_" + String(sessionId);
  _ensureFolder(sessionFolder.c_str());

  static int tlIdx = 0;
  String filename = _buildTimelapseFilename(sessionId, tlIdx++);
  String fullPath = sessionFolder + "/" + filename;

  File file = SD_MMC.open(fullPath, FILE_WRITE);
  if (!file) return "";
  file.write(data, len);
  file.close();

  Serial.printf("[SD] Timelapse: %s (%s)\n", fullPath.c_str(), formatSize(len).c_str());
  return filename;
}

// ─── Info ────────────────────────────────────────────────────
int Storage::getPhotoCount() {
  return max(0, _photoIndex - 1);
}

uint64_t Storage::getTotalSpaceBytes() {
  return _ready ? SD_MMC.totalBytes() : 0;
}

uint64_t Storage::getUsedSpaceBytes() {
  return _ready ? SD_MMC.usedBytes() : 0;
}

uint64_t Storage::getFreeSpaceBytes() {
  return _ready ? (SD_MMC.totalBytes() - SD_MMC.usedBytes()) : 0;
}

String Storage::formatSize(uint64_t bytes) {
  if (bytes < 1024)              return String((int)bytes) + " B";
  if (bytes < 1024 * 1024)      return String(bytes / 1024) + " KB";
  if (bytes < 1024*1024*1024ULL) return String(bytes / (1024*1024)) + " MB";
  return String(bytes / (1024*1024*1024ULL)) + " GB";
}

bool Storage::isReady()  { return _ready; }

bool Storage::folderExists(const char* path) {
  File f = SD_MMC.open(path);
  bool exists = f && f.isDirectory();
  f.close();
  return exists;
}

void Storage::listPhotos() {
  Serial.println("[SD] Listing photos:");
  File dir = SD_MMC.open(PHOTO_FOLDER);
  if (!dir || !dir.isDirectory()) {
    Serial.println("[SD] No photos folder");
    return;
  }
  File entry = dir.openNextFile();
  int count = 0;
  while (entry) {
    if (!entry.isDirectory()) {
      Serial.printf("  %s (%s)\n", entry.name(), formatSize(entry.size()).c_str());
      count++;
    }
    entry = dir.openNextFile();
  }
  Serial.printf("[SD] Total: %d photos\n", count);
}

// ─── Delete ──────────────────────────────────────────────────
bool Storage::deletePhoto(const String& filename) {
  String fullPath = String(PHOTO_FOLDER) + "/" + filename;
  bool ok = SD_MMC.remove(fullPath);
  Serial.printf("[SD] Delete %s: %s\n", fullPath.c_str(), ok ? "OK" : "FAILED");
  return ok;
}

bool Storage::deleteAll() {
  Serial.println("[SD] Deleting all photos...");
  File dir = SD_MMC.open(PHOTO_FOLDER);
  if (!dir || !dir.isDirectory()) return false;
  File entry = dir.openNextFile();
  int count = 0;
  while (entry) {
    if (!entry.isDirectory()) {
      String path = String(PHOTO_FOLDER) + "/" + entry.name();
      SD_MMC.remove(path);
      count++;
    }
    entry = dir.openNextFile();
  }
  _photoIndex = 1;
  Serial.printf("[SD] Deleted %d photos\n", count);
  return true;
}

// ─── Private ─────────────────────────────────────────────────
void Storage::_ensureFolder(const char* path) {
  if (!SD_MMC.exists(path)) {
    if (SD_MMC.mkdir(path)) {
      Serial.printf("[SD] Created folder: %s\n", path);
    } else {
      Serial.printf("[SD] Failed to create folder: %s\n", path);
    }
  }
}

int Storage::_scanHighestIndex() {
  File dir = SD_MMC.open(PHOTO_FOLDER);
  if (!dir || !dir.isDirectory()) return 0;

  int highest = 0;
  File entry = dir.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      // Extract number from e.g. "img_0042.jpg"
      int dot = name.lastIndexOf('.');
      if (dot > 0) {
        String numStr = name.substring(strlen(FILENAME_PREFIX), dot);
        int num = numStr.toInt();
        if (num > highest) highest = num;
      }
    }
    entry = dir.openNextFile();
  }
  return highest;
}

String Storage::_buildFilename(int idx) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s%04d.jpg", FILENAME_PREFIX, idx);
  return String(buf);
}

String Storage::_buildTimelapseFilename(int sessionId, int idx) {
  char buf[32];
  snprintf(buf, sizeof(buf), "frame_%04d.jpg", idx);
  return String(buf);
}
