#pragma once

#include <Arduino.h>
#include "FS.h"
#include "SD_MMC.h"
#include "config.h"

class Storage {
public:
  Storage();

  bool    begin();
  void    end();

  // Save a JPEG buffer to the next available filename
  // Returns the filename written, or "" on error
  String  savePhoto(const uint8_t* data, size_t len);

  // Timelapse naming
  String  saveTimelapse(const uint8_t* data, size_t len, int sessionId);

  // Info
  int     getPhotoCount();
  uint64_t getTotalSpaceBytes();
  uint64_t getUsedSpaceBytes();
  uint64_t getFreeSpaceBytes();
  String  formatSize(uint64_t bytes);

  bool    isReady();
  bool    folderExists(const char* path);
  void    listPhotos();

  // Delete
  bool    deletePhoto(const String& filename);
  bool    deleteAll();

private:
  bool    _ready;
  int     _photoIndex;    // Next photo number

  void    _ensureFolder(const char* path);
  int     _scanHighestIndex();
  String  _buildFilename(int idx);
  String  _buildTimelapseFilename(int sessionId, int idx);
};
