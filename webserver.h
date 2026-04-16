#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "camera.h"
#include "storage.h"

class PhotoWebServer {
public:
  PhotoWebServer(Camera* cam, Storage* storage);

  bool  begin();
  void  handle();          // Call in loop()
  String getIP();
  bool  isConnected();

private:
  WebServer  _server;
  Camera*    _cam;
  Storage*   _storage;
  bool       _connected;

  void _connectWiFi();
  void _setupRoutes();

  // Handlers
  void _handleRoot();
  void _handleCapture();
  void _handleStream();
  void _handleGallery();
  void _handleDownload();
  void _handleDelete();
  void _handleInfo();
  void _handleNotFound();

  // HTML helpers
  String _buildPage(const String& title, const String& body);
  String _buildGalleryPage();
};
