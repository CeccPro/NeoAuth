/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: turnstile_mode.cpp
 * Author: CeccPro
 */

#include "turnstile_mode.h"

TurnstileMode::TurnstileMode(SideConn* sideconn, APIClient* apiClient)
  : sideconn(sideconn), apiClient(apiClient), autoLockDelay(5000), unlockTime(0), isUnlocked(false), onAccessEventCallback(nullptr) {
}

void TurnstileMode::begin() {
  Serial.println("[TurnstileMode] Initializing turnstile mode");
  
  // Asegurarse de que el torniquete esté bloqueado al inicio
  lockTurnstile();
  
  // Test de conectividad con el slave
  if (pingSlave()) {
    Serial.println("[TurnstileMode] Slave connection OK");
  } else {
    Serial.println("[TurnstileMode] WARNING: Slave not responding");
  }
}

bool TurnstileMode::sendCommand(uint8_t cmd) {
  return sendCommand(cmd, 0x0000);
}

bool TurnstileMode::sendCommand(uint8_t cmd, uint16_t param) {
  // Formato: [CMD] [PARAM_HIGH] [PARAM_LOW] [RESERVED]
  uint8_t message[4] = {
    cmd,
    (uint8_t)((param >> 8) & 0xFF),  // High byte
    (uint8_t)(param & 0xFF),          // Low byte
    0x00                               // Reserved
  };
  
  bool success = sideconn->sendMessage(message);
  
  if (success) {
    Serial.printf("[TurnstileMode] Sent command: 0x%02X with param: 0x%04X\n", cmd, param);
  } else {
    Serial.println("[TurnstileMode] ERROR: Failed to send command");
  }
  
  return success;
}

bool TurnstileMode::unlockTurnstile() {
  // Usar el delay configurado (convertir de ms a segundos)
  uint16_t seconds = (uint16_t)(autoLockDelay / 1000);
  return unlockTurnstile(seconds);
}

bool TurnstileMode::unlockTurnstile(uint16_t seconds) {
  if (seconds == 0) {
    Serial.println("[TurnstileMode] Unlocking turnstile INDEFINITELY");
  } else {
    Serial.printf("[TurnstileMode] Unlocking turnstile for %u seconds\n", seconds);
  }
  
  if (sendCommand(TurnstileCmd::CMD_UNLOCK, seconds)) {
    isUnlocked = true;
    unlockTime = millis();
    
    // Si es indefinido, ajustar autoLockDelay a 0 para desactivar el auto-lock local
    if (seconds == 0) {
      autoLockDelay = 0;
    } else {
      // Actualizar autoLockDelay basado en los segundos enviados
      autoLockDelay = seconds * 1000UL;
    }
    
    return true;
  }
  
  return false;
}

bool TurnstileMode::lockTurnstile() {
  Serial.println("[TurnstileMode] Locking turnstile");
  
  if (sendCommand(TurnstileCmd::CMD_LOCK)) {
    isUnlocked = false;
    unlockTime = 0;
    return true;
  }
  
  return false;
}

bool TurnstileMode::pingSlave() {
  return sendCommand(TurnstileCmd::CMD_PING);
}

void TurnstileMode::setAutoLockDelay(unsigned long delayMs) {
  autoLockDelay = delayMs;
  Serial.println("[TurnstileMode] Auto-lock delay set to " + String(delayMs) + "ms");
}

bool TurnstileMode::isCardAuthorized(const String& uid) {
  // Verificar en la lista local
  return authorizedCards.find(uid) != authorizedCards.end();
}

void TurnstileMode::handleCardDetected(const String& uid) {
  Serial.println("[TurnstileMode] Card detected: " + uid);
  
  bool granted = false;
  
  // Si la API está habilitada, usarla para validar acceso
  if (apiClient && apiClient->isEnabled()) {
    Serial.println("[TurnstileMode] Validating access via API...");
    
    String userName = "";
    JsonObject userMetadata;
    
    if (apiClient->validateAccess(uid, granted, userName, userMetadata)) {
      if (granted) {
        Serial.println("[TurnstileMode] API: Access granted for " + userName);
        onAccessGranted(uid);
      } else {
        Serial.println("[TurnstileMode] API: Access denied");
        onAccessDenied(uid);
      }
      return;
    } else {
      // Si falla la API, denegar por seguridad
      Serial.println("[TurnstileMode] API validation failed - access denied by default");
      onAccessDenied(uid);
      return;
    }
  }
  
  // Fallback: usar lista local si la API no está habilitada
  Serial.println("[TurnstileMode] Validating access via local list...");
  if (isCardAuthorized(uid)) {
    onAccessGranted(uid);
  } else {
    onAccessDenied(uid);
  }
}

void TurnstileMode::setOnAccessEventCallback(OnAccessEventCallback callback) {
  onAccessEventCallback = callback;
}

void TurnstileMode::onAccessGranted(const String& uid) {
  Serial.println("[TurnstileMode] Access GRANTED for card: " + uid);
  unlockTurnstile();
  
  // Notificar al callback si existe
  if (onAccessEventCallback) {
    onAccessEventCallback(uid, true);
  }
}

void TurnstileMode::onAccessDenied(const String& uid) {
  Serial.println("[TurnstileMode] Access DENIED for card: " + uid);
  
  // Notificar al callback si existe
  if (onAccessEventCallback) {
    onAccessEventCallback(uid, false);
  }
}

void TurnstileMode::periodicTask() {
  // Auto-bloquear el torniquete después del delay configurado
  // Si autoLockDelay es 0, no hacer auto-lock (desbloqueo indefinido)
  if (isUnlocked && autoLockDelay > 0 && (millis() - unlockTime >= autoLockDelay)) {
    Serial.println("[TurnstileMode] Auto-lock timeout reached");
    lockTurnstile();
  }
}

void TurnstileMode::addAuthorizedCard(const String& uid) {
  authorizedCards.insert(uid);
  Serial.println("[TurnstileMode] Added authorized card: " + uid);
}

void TurnstileMode::removeAuthorizedCard(const String& uid) {
  authorizedCards.erase(uid);
  Serial.println("[TurnstileMode] Removed authorized card: " + uid);
}

void TurnstileMode::clearAuthorizedCards() {
  authorizedCards.clear();
  Serial.println("[TurnstileMode] Cleared all authorized cards");
}

std::vector<String> TurnstileMode::getAuthorizedCards() const {
  return std::vector<String>(authorizedCards.begin(), authorizedCards.end());
}
