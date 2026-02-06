/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: time_manager.h
 * Author: CeccPro
 * 
 * Description:
 *   Time Manager - Manages time sync with online API
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

struct TimeInfo {
  String time;      // "HH:MM:SS"
  String date;      // "YYYY-MM-DD"
  String day;       // "monday", "tuesday", etc.
  bool valid;
};

class TimeManager {
public:
  TimeManager(const String& apiBaseURL);
  
  // Sincronizar hora con el servidor
  bool syncTime();
  
  // Obtener hora actual (usa cache si es reciente)
  TimeInfo getCurrentTime(bool forceSync = false);
  
  // Verificar si el cache es válido
  bool isCacheValid() const;
  
  // Configuración
  void setBaseURL(const String& url);
  void setCacheTimeout(unsigned long timeoutMs); // Default: 60000ms (1 minuto)
  
  // Estado
  bool isTimeSynced() const { return lastSyncTime > 0; }
  unsigned long getTimeSinceLastSync() const;

private:
  String apiBaseURL;
  WiFiClientSecure wifiClient;
  
  // Cache de tiempo
  TimeInfo cachedTime;
  unsigned long lastSyncTime;      // millis() cuando se hizo el último sync
  unsigned long cacheTimeout;      // Tiempo de validez del cache
  
  // Estimación de tiempo basado en millis()
  TimeInfo estimateTime() const;
  String addSecondsToTime(const String& time, int seconds) const;
  int timeToSeconds(const String& time) const;
  String secondsToTime(int seconds) const;
};

#endif // TIME_MANAGER_H
