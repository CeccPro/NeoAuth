/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: turnstile_mode.cpp
 * Author: CeccPro
 */

#include "turnstile_mode.h"

TurnstileMode::TurnstileMode(I2CComm* i2cComm, APIClient* apiClient)
  : i2cComm(i2cComm), apiClient(apiClient), autoLockDelay(5000), unlockTime(0), isUnlocked(false) {
}

void TurnstileMode::begin() {
  Serial.println("[TurnstileMode] Initializing turnstile mode");
  Serial.println("[TurnstileMode] ====================================");
  
  // Test de conectividad con el slave
  Serial.println("[TurnstileMode] Testing slave connection...");
  if (pingSlave()) {
    Serial.println("[TurnstileMode] ✓ Slave connection OK");
  } else {
    Serial.println("[TurnstileMode] ✗ WARNING: Slave not responding");
    Serial.println("[TurnstileMode]   Check slave device connection");
  }
  
  // Asegurarse de que el torniquete esté bloqueado al inicio
  Serial.println("[TurnstileMode] Setting initial state to LOCKED");
  
  // Dar tiempo extra después del ping antes de enviar lock
  delay(50);
  
  lockTurnstile();
  
  Serial.println("[TurnstileMode] ====================================");
  Serial.println("[TurnstileMode] Initialization complete");
}

bool TurnstileMode::sendCommand(uint8_t cmd) {
  bool success = i2cComm->sendCommand(cmd);
  
  if (success) {
    Serial.println("[TurnstileMode] Sent command: 0x" + String(cmd, HEX));
  } else {
    Serial.println("[TurnstileMode] ERROR: Failed to send command");
  }
  
  return success;
}

bool TurnstileMode::unlockTurnstile() {
  Serial.println("[TurnstileMode] Unlocking turnstile");
  
  if (sendCommand(TurnstileCmd::CMD_UNLOCK)) {
    isUnlocked = true;
    unlockTime = millis();
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
  Serial.println("[TurnstileMode] Sending PING to slave...");
  
  uint8_t recvMsg[4] = {0x00, 0x00, 0x00, 0x00};
  
  // Enviar ping y esperar respuesta
  if (i2cComm->sendAndReceive(TurnstileCmd::CMD_PING, recvMsg, 4, 200)) {
    // Verificar que la respuesta sea válida (debe ser 0xAA en el primer byte)
    if (recvMsg[0] == 0xAA) {
      Serial.println("[TurnstileMode] PONG received from slave!");
      Serial.print("[TurnstileMode] Response: ");
      for (int i = 0; i < 4; i++) {
        Serial.print("0x");
        Serial.print(recvMsg[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      return true;
    } else {
      Serial.print("[TurnstileMode] Invalid PONG response: 0x");
      Serial.println(recvMsg[0], HEX);
      return false;
    }
  }
  
  Serial.println("[TurnstileMode] No PONG response from slave");
  return false;
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
  
  // Si API está habilitada, validar con API
  if (apiClient && apiClient->isEnabled()) {
    bool accessGranted = false;
    String userName = "";
    
    if (apiClient->validateAccess(uid, accessGranted, userName)) {
      if (accessGranted) {
        Serial.println("[TurnstileMode] ✓ Access granted for: " + userName);
        onAccessGranted(uid);
      } else {
        Serial.println("[TurnstileMode] ✗ Access denied");
        onAccessDenied(uid);
      }
      return;
    }
    
    // Si falla la API, denegar por seguridad
    Serial.println("[TurnstileMode] ✗ API error - access denied for security");
    onAccessDenied(uid);
    return;
  }
  
  // Modo local (fallback si API no disponible)
  if (isCardAuthorized(uid)) {
    onAccessGranted(uid);
  } else {
    onAccessDenied(uid);
  }
}

void TurnstileMode::onAccessGranted(const String& uid) {
  Serial.println("[TurnstileMode] Access GRANTED for card: " + uid);
  unlockTurnstile();
}

void TurnstileMode::onAccessDenied(const String& uid) {
  Serial.println("[TurnstileMode] Access DENIED for card: " + uid);
  // No hacer nada, el torniquete permanece bloqueado
}

void TurnstileMode::periodicTask() {
  // Auto-bloquear el torniquete después del delay configurado
  if (isUnlocked && (millis() - unlockTime >= autoLockDelay)) {
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
