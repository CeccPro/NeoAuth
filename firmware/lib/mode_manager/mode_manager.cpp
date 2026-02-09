/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: mode_manager.cpp
 * Author: CeccPro
 */

#include "mode_manager.h"
#include <SPIFFS.h>

const char* modeToString(SensorMode mode) {
  switch (mode) {
    case MODE_STANDALONE:   return "standalone";
    case MODE_TURNSTILE:    return "turnstile";
    case MODE_REGISTRATION: return "registration";
    case MODE_SYNC:         return "sync";
    default:                return "unknown";
  }
}

SensorMode stringToMode(const String& modeStr) {
  if (modeStr == "standalone")   return MODE_STANDALONE;
  if (modeStr == "turnstile")    return MODE_TURNSTILE;
  if (modeStr == "registration") return MODE_REGISTRATION;
  if (modeStr == "sync")         return MODE_SYNC;
  return MODE_STANDALONE; // Default
}

ModeManager::ModeManager() : currentMode(MODE_STANDALONE) {
}

void ModeManager::begin() {
  // Intentar cargar el modo guardado
  if (!loadMode()) {
    Serial.println("[ModeManager] No saved mode found, using STANDALONE");
    currentMode = MODE_STANDALONE;
  }
  
  Serial.println("[ModeManager] Current mode: " + String(modeToString(currentMode)));
}

bool ModeManager::setMode(SensorMode newMode) {
  Serial.println("[ModeManager] Changing mode from " + String(modeToString(currentMode)) + 
                 " to " + String(modeToString(newMode)));
  
  currentMode = newMode;
  
  // Guardar el nuevo modo
  return saveMode();
}

bool ModeManager::saveMode() {
  File file = SPIFFS.open(CONFIG_MODE_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("[ModeManager] ERROR: Failed to open mode file for writing");
    return false;
  }
  
  file.println(modeToString(currentMode));
  file.close();
  
  Serial.println("[ModeManager] Mode saved: " + String(modeToString(currentMode)));
  return true;
}

bool ModeManager::loadMode() {
  if (!SPIFFS.exists(CONFIG_MODE_FILE)) {
    return false;
  }
  
  File file = SPIFFS.open(CONFIG_MODE_FILE, FILE_READ);
  if (!file) {
    Serial.println("[ModeManager] ERROR: Failed to open mode file for reading");
    return false;
  }
  
  String modeStr = file.readStringUntil('\n');
  modeStr.trim();
  file.close();
  
  currentMode = stringToMode(modeStr);
  Serial.println("[ModeManager] Mode loaded: " + String(modeToString(currentMode)));
  
  return true;
}
