/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: standalone_mode.h
 * Author: CeccPro
 * 
 * Description:
 *   Standalone Mode - Identification mode for RFID cards
 */

#ifndef STANDALONE_MODE_H
#define STANDALONE_MODE_H

#include <Arduino.h>
#include <api_client.h>
#include <functional>

class StandaloneMode {
public:
  StandaloneMode(APIClient* apiClient);
  
  // Inicialización
  void begin();
  
  // Procesar tarjeta detectada
  void handleCardDetected(const String& uid);
  
  // Callback para notificar eventos de identificación
  typedef std::function<void(const String& uid, bool found, const String& userName, const String& userEmail)> OnIdentificationCallback;
  void setOnIdentificationCallback(OnIdentificationCallback callback);

private:
  APIClient* apiClient;
  OnIdentificationCallback onIdentificationCallback;
};

#endif // STANDALONE_MODE_H
