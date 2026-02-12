/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: admin_mode.cpp
 * Author: CeccPro
 */

#include "admin_mode.h"

AdminMode::AdminMode(APIClient* apiClient)
  : apiClient(apiClient), onCardDetectedCallback(nullptr) {
}

void AdminMode::begin() {
  Serial.println("[AdminMode] Initializing admin mode");
  Serial.println("[AdminMode] Mode: Card and user management");
  Serial.println("[AdminMode] Ready for card detection");
}

void AdminMode::setOnCardDetectedCallback(OnCardDetectedCallback callback) {
  onCardDetectedCallback = callback;
}

void AdminMode::handleCardDetected(const String& uid) {
  Serial.println("[AdminMode] Card detected: " + uid);
  
  if (!apiClient || !apiClient->isEnabled()) {
    Serial.println("[AdminMode] API not configured - admin mode requires API");
    
    // Crear cardInfo vacía
    CardInfo cardInfo;
    cardInfo.uid = uid;
    cardInfo.userId = "";
    cardInfo.userName = "";
    cardInfo.userEmail = "";
    cardInfo.role = "";
    cardInfo.isActive = false;
    
    if (onCardDetectedCallback) {
      onCardDetectedCallback(cardInfo, false);
    }
    return;
  }
  
  CardInfo cardInfo;
  bool found = fetchCardInfo(uid, cardInfo);
  
  if (found) {
    Serial.println("[AdminMode] ✓ Card found in database:");
    Serial.println("  User: " + cardInfo.userName);
    Serial.println("  Role: " + cardInfo.role);
    Serial.println("  Active: " + String(cardInfo.isActive ? "Yes" : "No"));
  } else {
    Serial.println("[AdminMode] ✗ Card not registered - new card");
  }
  
  // Notificar al callback
  if (onCardDetectedCallback) {
    onCardDetectedCallback(cardInfo, found);
  }
}

bool AdminMode::fetchCardInfo(const String& uid, CardInfo& cardInfo) {
  // Usar whoIs para obtener información básica
  bool found = false;
  String userName = "";
  String userEmail = "";
  JsonObject userMetadata;
  
  if (!apiClient->whoIs(uid, found, userName, userEmail, userMetadata)) {
    Serial.println("[AdminMode] API error fetching card info");
    return false;
  }
  
  if (!found) {
    // Tarjeta no existe
    cardInfo.uid = uid;
    cardInfo.userId = "";
    cardInfo.userName = "";
    cardInfo.userEmail = "";
    cardInfo.role = "";
    cardInfo.isActive = false;
    return false;
  }
  
  // Extraer información
  cardInfo.uid = uid;
  cardInfo.userName = userName;
  cardInfo.userEmail = userEmail;
  
  // TODO: La API debe devolver más info (user_id, role, is_active, metadata)
  // Por ahora usar valores por defecto
  cardInfo.userId = "";  // TODO: Obtener de API
  cardInfo.role = userMetadata["role"] | "user";
  cardInfo.isActive = true;  // TODO: Obtener de API
  cardInfo.metadata = userMetadata;
  
  return true;
}
