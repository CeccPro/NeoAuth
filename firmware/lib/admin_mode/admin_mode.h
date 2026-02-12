/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: admin_mode.h
 * Author: CeccPro
 * 
 * Description:
 *   Admin Mode - Manage RFID cards and users from the sensor
 */

#ifndef ADMIN_MODE_H
#define ADMIN_MODE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <api_client.h>
#include <functional>

// Estructura para información de tarjeta
struct CardInfo {
  String uid;
  String userId;
  String userName;
  String userEmail;
  String role;
  bool isActive;
  JsonObject metadata;
};

class AdminMode {
public:
  AdminMode(APIClient* apiClient);
  
  // Inicialización
  void begin();
  
  // Procesar tarjeta detectada en modo admin
  void handleCardDetected(const String& uid);
  
  // Callbacks para notificar eventos
  typedef std::function<void(const CardInfo& cardInfo, bool found)> OnCardDetectedCallback;
  void setOnCardDetectedCallback(OnCardDetectedCallback callback);
  
private:
  APIClient* apiClient;
  OnCardDetectedCallback onCardDetectedCallback;
  
  // Helper: Consultar información de tarjeta
  bool fetchCardInfo(const String& uid, CardInfo& cardInfo);
};

#endif // ADMIN_MODE_H
