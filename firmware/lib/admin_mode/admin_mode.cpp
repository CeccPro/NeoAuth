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
    Serial.println("  UID: " + cardInfo.uid);
  }
  
  // Notificar al callback (SIEMPRE, incluso si no se encontró)
  if (onCardDetectedCallback) {
    Serial.println("[AdminMode] Calling callback with UID: " + cardInfo.uid + " Found: " + String(found ? "YES" : "NO"));
    onCardDetectedCallback(cardInfo, found);
  } else {
    Serial.println("[AdminMode] WARNING: No callback registered!");
  }
}

bool AdminMode::fetchCardInfo(const String& uid, CardInfo& cardInfo) {
  // Usar getCardInfo para obtener información completa
  bool found = false;
  String userId = "";
  String userName = "";
  String userEmail = "";
  String role = "";
  bool isActive = false;
  
  DynamicJsonDocument metadataDoc(512);
  JsonObject userMetadata = metadataDoc.to<JsonObject>();
  
  if (!apiClient->getCardInfo(uid, found, userId, userName, userEmail, role, isActive, userMetadata)) {
    Serial.println("[AdminMode] API error fetching card info");
    // Error de API - llenar cardInfo con UID y retornar false
    cardInfo.uid = uid;
    cardInfo.userId = "";
    cardInfo.userName = "";
    cardInfo.userEmail = "";
    cardInfo.role = "";
    cardInfo.isActive = false;
    cardInfo.metadata = "{}";
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
    cardInfo.metadata = "{}";
    return false;
  }
  
  // Extraer información
  cardInfo.uid = uid;
  cardInfo.userId = userId;
  cardInfo.userName = userName;
  cardInfo.userEmail = userEmail;
  cardInfo.role = role;
  cardInfo.isActive = isActive;
  
  // Serializar metadata a String
  String metadataStr;
  serializeJson(userMetadata, metadataStr);
  cardInfo.metadata = metadataStr;
  
  return true;
}
