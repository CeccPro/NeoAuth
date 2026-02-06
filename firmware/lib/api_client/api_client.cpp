/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: api_client.cpp
 * Author: CeccPro
 */

#include "api_client.h"
#include <mbedtls/md.h>

APIClient::APIClient(const String& sensorId, const String& authSecret)
  : sensorId(sensorId), authSecret(authSecret), enabled(false), timeout(10000) {
  
  // Generar token de autenticación
  authToken = generateAuthToken();
  
  // Configurar SSL para aceptar certificados (inseguro, pero necesario para muchos casos)
  wifiClient.setInsecure();
}

void APIClient::setBaseURL(const String& url) {
  baseURL = url;
  if (baseURL.endsWith("/")) {
    baseURL.remove(baseURL.length() - 1);
  }
}

void APIClient::setEnabled(bool en) {
  enabled = en;
}

void APIClient::setTimeout(unsigned long timeoutMs) {
  timeout = timeoutMs;
}

String APIClient::generateAuthToken() {
  // Generar HMAC-SHA256(auth_secret, sensor_id)
  byte hmacResult[32];
  
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)authSecret.c_str(), authSecret.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*)sensorId.c_str(), sensorId.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
  
  // Convertir a hexadecimal
  String token = "";
  for (int i = 0; i < 32; i++) {
    char hex[3];
    sprintf(hex, "%02x", hmacResult[i]);
    token += hex;
  }
  
  return token;
}

bool APIClient::makeRequest(const String& endpoint, const JsonDocument& payload, JsonDocument& response) {
  if (!enabled) {
    Serial.println("[APIClient] API is disabled");
    return false;
  }
  
  if (baseURL.isEmpty()) {
    Serial.println("[APIClient] Base URL not configured");
    return false;
  }
  
  String url = baseURL + endpoint;
  
  HTTPClient http;
  http.begin(wifiClient, url);
  http.setTimeout(timeout);
  http.addHeader("Content-Type", "application/json");
  
  // Serializar payload
  String jsonPayload;
  serializeJson(payload, jsonPayload);
  
  Serial.println("[APIClient] POST " + endpoint);
  Serial.println("[APIClient] Payload: " + jsonPayload);
  
  // Hacer request
  int httpCode = http.POST(jsonPayload);
  
  if (httpCode > 0) {
    Serial.printf("[APIClient] Response code: %d\n", httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      String responseStr = http.getString();
      Serial.println("[APIClient] Response: " + responseStr);
      
      DeserializationError error = deserializeJson(response, responseStr);
      if (error) {
        Serial.println("[APIClient] Failed to parse JSON response");
        http.end();
        return false;
      }
      
      http.end();
      return true;
    } else {
      Serial.printf("[APIClient] HTTP error: %d\n", httpCode);
      String errorResponse = http.getString();
      Serial.println("[APIClient] Error: " + errorResponse);
    }
  } else {
    Serial.printf("[APIClient] Connection failed: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
  return false;
}

bool APIClient::sendHeartbeat() {
  DynamicJsonDocument payload(256);
  payload["sensor_id"] = sensorId;
  payload["auth_token"] = authToken;
  
  DynamicJsonDocument response(512);
  
  if (makeRequest("/api/v1/heartbeat", payload, response)) {
    String status = response["status"] | "";
    if (status == "ok") {
      Serial.println("[APIClient] Heartbeat OK");
      return true;
    }
  }
  
  return false;
}

bool APIClient::validateAccess(const String& uid, bool& accessGranted, String& userName, JsonObject& userMetadata) {
  DynamicJsonDocument payload(512);
  payload["sensor_id"] = sensorId;
  payload["auth_token"] = authToken;
  payload["uid"] = uid;
  payload["timestamp"] = String(millis());
  
  DynamicJsonDocument response(1024);
  
  if (makeRequest("/api/v1/access", payload, response)) {
    String status = response["status"] | "";
    
    if (status == "ok") {
      accessGranted = true;
      userName = response["user"]["name"] | "Unknown";
      
      // Copiar metadata si existe
      if (!response["user"]["metadata"].isNull()) {
        userMetadata = response["user"]["metadata"].as<JsonObject>();
      }
      
      Serial.println("[APIClient] Access granted for: " + userName);
      return true;
    } else if (status == "denied") {
      accessGranted = false;
      userName = "";
      Serial.println("[APIClient] Access denied");
      return true;
    }
  }
  
  // Si falla la API, denegar acceso por seguridad
  accessGranted = false;
  userName = "";
  return false;
}

bool APIClient::whoIs(const String& uid, bool& found, String& userName, String& userEmail, JsonObject& userMetadata) {
  DynamicJsonDocument payload(512);
  payload["sensor_id"] = sensorId;
  payload["auth_token"] = authToken;
  payload["uid"] = uid;
  payload["timestamp"] = String(millis());
  
  DynamicJsonDocument response(1024);
  
  if (makeRequest("/api/v1/who_is", payload, response)) {
    String status = response["status"] | "";
    
    if (status == "ok") {
      found = true;
      userName = response["user"]["name"] | "Unknown";
      userEmail = response["user"]["email"] | "";
      userMetadata = response["user"]["metadata"] | JsonObject();
      Serial.println("[APIClient] User found: " + userName);
      return true;
    } else if (status == "not_found" || status == "inactive") {
      found = false;
      userName = "";
      userEmail = "";
      Serial.println("[APIClient] User not found");
      return true;
    }
  }
  
  found = false;
  return false;
}

bool APIClient::testConnection() {
  Serial.println("[APIClient] Testing connection to: " + baseURL);
  return sendHeartbeat();
}
