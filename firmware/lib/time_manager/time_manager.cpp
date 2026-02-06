/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: time_manager.cpp
 * Author: CeccPro
 */

#include "time_manager.h"

TimeManager::TimeManager(const String& apiBaseURL)
  : apiBaseURL(apiBaseURL), lastSyncTime(0), cacheTimeout(60000) {
  
  wifiClient.setInsecure(); // Aceptar certificados SSL
  
  cachedTime.valid = false;
}

void TimeManager::setBaseURL(const String& url) {
  apiBaseURL = url;
  if (apiBaseURL.endsWith("/")) {
    apiBaseURL.remove(apiBaseURL.length() - 1);
  }
}

void TimeManager::setCacheTimeout(unsigned long timeoutMs) {
  cacheTimeout = timeoutMs;
}

bool TimeManager::syncTime() {
  if (apiBaseURL.isEmpty()) {
    Serial.println("[TimeManager] Base URL not configured");
    return false;
  }
  
  String url = apiBaseURL + "/api/v1/time";
  
  HTTPClient http;
  http.begin(wifiClient, url);
  http.setTimeout(5000);
  
  Serial.println("[TimeManager] Syncing time with server...");
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.println("[TimeManager] Failed to parse response");
      http.end();
      return false;
    }
    
    // Actualizar cache
    cachedTime.time = doc["time"] | "";
    cachedTime.date = doc["date"] | "";
    cachedTime.day = doc["day"] | "";
    cachedTime.valid = true;
    
    lastSyncTime = millis();
    
    Serial.println("[TimeManager] ✓ Time synced:");
    Serial.println("  Time: " + cachedTime.time);
    Serial.println("  Date: " + cachedTime.date);
    Serial.println("  Day: " + cachedTime.day);
    
    http.end();
    return true;
  } else {
    Serial.printf("[TimeManager] ✗ Sync failed: HTTP %d\n", httpCode);
    http.end();
    return false;
  }
}

TimeInfo TimeManager::getCurrentTime(bool forceSync) {
  // Si se fuerza sync o el cache no es válido, sincronizar
  if (forceSync || !isCacheValid()) {
    if (syncTime()) {
      return cachedTime;
    }
    
    // Si falla el sync y hay un cache previo, usar estimación
    if (lastSyncTime > 0) {
      Serial.println("[TimeManager] Using estimated time from last sync");
      return estimateTime();
    }
    
    // No hay tiempo disponible
    TimeInfo invalid = {"00:00:00", "1970-01-01", "unknown", false};
    return invalid;
  }
  
  // Cache válido, retornar estimación basada en millis()
  return estimateTime();
}

bool TimeManager::isCacheValid() const {
  if (lastSyncTime == 0) return false;
  
  unsigned long elapsed = millis() - lastSyncTime;
  
  // Protección contra overflow de millis()
  if (elapsed > 0x7FFFFFFF) return false;
  
  return elapsed < cacheTimeout;
}

unsigned long TimeManager::getTimeSinceLastSync() const {
  if (lastSyncTime == 0) return 0xFFFFFFFF;
  return millis() - lastSyncTime;
}

// ============================================================================
// ESTIMACIÓN DE TIEMPO
// ============================================================================

TimeInfo TimeManager::estimateTime() const {
  if (!cachedTime.valid || lastSyncTime == 0) {
    TimeInfo invalid = {"00:00:00", "1970-01-01", "unknown", false};
    return invalid;
  }
  
  // Calcular segundos transcurridos desde el último sync
  unsigned long elapsedMs = millis() - lastSyncTime;
  int elapsedSeconds = elapsedMs / 1000;
  
  // Agregar segundos al tiempo cacheado
  TimeInfo estimated = cachedTime;
  estimated.time = addSecondsToTime(cachedTime.time, elapsedSeconds);
  
  // Nota: No actualizamos date/day porque sería complejo sin librería de fechas
  // Para una solución más robusta, usar una librería de tiempo real
  
  return estimated;
}

String TimeManager::addSecondsToTime(const String& time, int seconds) const {
  int currentSeconds = timeToSeconds(time);
  int newSeconds = (currentSeconds + seconds) % 86400; // 86400 = segundos en un día
  return secondsToTime(newSeconds);
}

int TimeManager::timeToSeconds(const String& time) const {
  // Convertir "HH:MM:SS" a segundos
  int h = time.substring(0, 2).toInt();
  int m = time.substring(3, 5).toInt();
  int s = time.substring(6, 8).toInt();
  
  return h * 3600 + m * 60 + s;
}

String TimeManager::secondsToTime(int seconds) const {
  int h = seconds / 3600;
  int m = (seconds % 3600) / 60;
  int s = seconds % 60;
  
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", h, m, s);
  return String(buffer);
}
