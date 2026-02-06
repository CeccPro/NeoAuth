/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: mode_manager.cpp
 * Author: CeccPro
 */

#include "mode_manager.h"
#include "../config_manager/config_manager.h"

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

ModeManager::ModeManager(ConfigManager* configManager) 
  : currentMode(MODE_STANDALONE), configManager(configManager) {
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
  // Verificar si ya estamos en ese modo
  if (currentMode == newMode) {
    Serial.println("[ModeManager] Already in " + String(modeToString(newMode)) + " mode, no change needed");
    return false; // Retornar false para indicar que no hubo cambio
  }
  
  Serial.println("[ModeManager] Changing mode from " + String(modeToString(currentMode)) + 
                 " to " + String(modeToString(newMode)));
  
  currentMode = newMode;
  
  // Guardar el nuevo modo
  return saveMode();
}

bool ModeManager::saveMode() {
  if (!configManager) {
    Serial.println("[ModeManager] ERROR: ConfigManager not set");
    return false;
  }
  
  bool result = configManager->setMode(modeToString(currentMode));
  if (result) {
    Serial.println("[ModeManager] Mode saved: " + String(modeToString(currentMode)));
  }
  return result;
}

bool ModeManager::loadMode() {
  if (!configManager) {
    Serial.println("[ModeManager] ERROR: ConfigManager not set");
    return false;
  }
  
  String modeStr = configManager->getMode();
  currentMode = stringToMode(modeStr);
  Serial.println("[ModeManager] Mode loaded: " + String(modeToString(currentMode)));
  
  return true;
}
