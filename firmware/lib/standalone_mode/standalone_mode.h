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

class StandaloneMode {
public:
  StandaloneMode(APIClient* apiClient);
  
  // Inicialización
  void begin();
  
  // Procesar tarjeta detectada
  void handleCardDetected(const String& uid);

private:
  APIClient* apiClient;
};

#endif // STANDALONE_MODE_H
