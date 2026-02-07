/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: api_client.h
 * Author: CeccPro
 * 
 * Description:
 *   API Client for NeoAuth - Handles communication with remote server
 */

#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

class APIClient {
public:
  APIClient(const String& sensorId, const String& authSecret);
  
  // Configuración
  void setBaseURL(const String& url);
  void setEnabled(bool enabled);
  void setTimeout(unsigned long timeoutMs);
  
  bool isEnabled() const { return enabled; }
  String getBaseURL() const { return baseURL; }
  String getAuthToken() const { return authToken; }
  
  // Endpoints
  bool sendHeartbeat();
  bool sendHeartbeat(time_t& unixTime); // Versión que retorna el timestamp
  bool validateAccess(const String& uid, bool& accessGranted, String& userName, JsonObject& userMetadata);
  bool whoIs(const String& uid, bool& found, String& userName, String& userEmail, JsonObject& userMetadata);
  
  // Test de conectividad
  bool testConnection();
  
  // Hacer request genérico (para uso de otros módulos como ReportManager)
  bool makeRequest(const String& endpoint, const JsonDocument& payload, JsonDocument& response);

private:
  String sensorId;
  String authSecret;
  String baseURL;
  String authToken;
  bool enabled;
  unsigned long timeout;
  
  WiFiClientSecure wifiClient;
  
  // Generar token de autenticación HMAC-SHA256
  String generateAuthToken();
};

#endif // API_CLIENT_H
