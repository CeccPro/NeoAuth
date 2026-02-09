/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: web_server.cpp
 * Author: CeccPro
 * 
 * Description:
 *   Web Server Manager Implementation
 */

#include "web_server.h"
#include "../wifi_manager/wifi_manager.h"
#include "../config_manager/config_manager.h"
#include "../mode_manager/mode_manager.h"
#include "../turnstile_mode/turnstile_mode.h"
#include "../power/power.h"

WebServerManager::WebServerManager(uint16_t port)
  : server(port), ws("/ws"), wifiManager(nullptr), configManager(nullptr),
    modeManager(nullptr), turnstileMode(nullptr),
    sensorId(nullptr), firmwareVersion(nullptr) {
}

void WebServerManager::begin(WiFiManager* wifiMgr, ConfigManager* configMgr,
                             const char* sensorId, const char* firmwareVersion) {
  this->wifiManager = wifiMgr;
  this->configManager = configMgr;
  this->sensorId = sensorId;
  this->firmwareVersion = firmwareVersion;
  
  // Configurar WebSocket
  ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                    AwsEventType type, void *arg, uint8_t *data, size_t len) {
    this->handleWebSocketEvent(server, client, type, arg, data, len);
  });
  
  // Agregar el WebSocket al servidor
  server.addHandler(&ws);
  
  // Servir index.html desde SPIFFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  // Servir archivos estáticos (CSS, JS, fuentes)
  server.serveStatic("/", SPIFFS, "/");
  
  server.begin();
  Serial.println("Web server started");
}

void WebServerManager::setModeManager(ModeManager* modeMgr) {
  this->modeManager = modeMgr;
}

void WebServerManager::setTurnstileMode(TurnstileMode* turnstileModePtr) {
  this->turnstileMode = turnstileModePtr;
}

void WebServerManager::notifyClients(const String& message) {
  ws.textAll(message);
}

void WebServerManager::sendSystemInfo(AsyncWebSocketClient* client) {
  DynamicJsonDocument doc(1024);
  doc["type"] = "system_info";
  doc["sensor_id"] = sensorId;
  doc["firmware_version"] = firmwareVersion;
  doc["wifi_connected"] = wifiManager->isConnected();
  doc["wifi_ssid"] = wifiManager->getConnectedSSID();
  if (wifiManager->isConnected()) {
    doc["wifi_ip"] = wifiManager->getLocalIP().toString();
  }
  doc["ap_ip"] = wifiManager->getAPIP().toString();
  
  // Agregar modo actual si modeManager está disponible
  if (modeManager) {
    doc["current_mode"] = modeManager->getCurrentModeString();
  }
  
  String response;
  serializeJson(doc, response);
  
  if (client) {
    client->text(response);
  } else {
    notifyClients(response);
  }
}

void WebServerManager::sendScanResult(int networkCount) {
  if (networkCount < 0) {
    // Error en el escaneo
    DynamicJsonDocument doc(256);
    doc["type"] = "error";
    doc["message"] = "Error al escanear redes";
    String msg;
    serializeJson(doc, msg);
    notifyClients(msg);
    return;
  }
  
  // Preparar respuesta JSON con las redes encontradas
  DynamicJsonDocument response(2048);
  response["type"] = "scan_result";
  JsonArray networks = response.createNestedArray("networks");
  
  for (int i = 0; i < networkCount; i++) {
    JsonObject net = networks.createNestedObject();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
    net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Abierta" : "Protegida";
    yield(); // Alimentar watchdog
  }
  
  String json;
  serializeJson(response, json);
  notifyClients(json);
  
  // Limpiar resultados del escaneo
  WiFi.scanDelete();
}

void WebServerManager::notifyCardDetected(const String& uid) {
  DynamicJsonDocument doc(256);
  doc["type"] = "card";
  doc["uid"] = uid;
  doc["timestamp"] = millis();
  String jsonMsg;
  serializeJson(doc, jsonMsg);
  notifyClients(jsonMsg);
}

void WebServerManager::notifyWiFiConnected(const String& ssid, const String& ip) {
  DynamicJsonDocument doc(256);
  doc["type"] = "wifi_connected";
  doc["ssid"] = ssid;
  doc["ip"] = ip;
  String msg;
  serializeJson(doc, msg);
  notifyClients(msg);
}

void WebServerManager::notifyWiFiDisconnected() {
  DynamicJsonDocument doc(128);
  doc["type"] = "wifi_disconnected";
  String msg;
  serializeJson(doc, msg);
  notifyClients(msg);
}

void WebServerManager::notifyAccessEvent(const String& uid, bool granted) {
  DynamicJsonDocument doc(256);
  doc["type"] = "access_event";
  doc["uid"] = uid;
  doc["granted"] = granted;
  doc["timestamp"] = millis();
  String msg;
  serializeJson(doc, msg);
  notifyClients(msg);
}

void WebServerManager::periodicTask() {
  // Limpiar clientes WebSocket desconectados
  ws.cleanupClients();
}

void WebServerManager::handleWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                           AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Cliente WebSocket #%u conectado desde %s\n", 
                  client->id(), client->remoteIP().toString().c_str());
    sendSystemInfo(client);
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Cliente WebSocket #%u desconectado\n", client->id());
  }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len) {
      if (info->opcode == WS_TEXT) {
        String msg = "";
        for (size_t i = 0; i < len; i++) {
          msg += (char)data[i];
        }
        
        Serial.println("Comando recibido: " + msg);
        
        // Parsear comando JSON
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, msg);
        
        if (!error) {
          String command = doc["command"].as<String>();
          
          if (command == "getConfig") {
            handleGetConfig(client);
          }
          else if (command == "addWiFi") {
            String ssid = doc["ssid"].as<String>();
            String password = doc["password"].as<String>();
            handleAddWiFi(client, ssid, password);
          }
          else if (command == "deleteWiFi") {
            String ssid = doc["ssid"].as<String>();
            handleDeleteWiFi(client, ssid);
          }
          else if (command == "scanNetworks") {
            handleScanNetworks(client);
          }
          else if (command == "connectWiFi") {
            handleConnectWiFi(client);
          }
          else if (command == "setMode") {
            String mode = doc["mode"].as<String>();
            handleSetMode(client, mode);
          }
          else if (command == "getMode") {
            handleGetMode(client);
          }
          else if (command == "addAuthorizedCard") {
            String uid = doc["uid"].as<String>();
            handleAddAuthorizedCard(client, uid);
          }
          else if (command == "removeAuthorizedCard") {
            String uid = doc["uid"].as<String>();
            handleRemoveAuthorizedCard(client, uid);
          }
          else if (command == "getAuthorizedCards") {
            handleGetAuthorizedCards(client);
          }
          else if (command == "unlockTurnstile") {
            uint16_t duration = doc["duration"] | 0;  // Segundos, 0 = usar default o indefinido
            handleUnlockTurnstile(client, duration);
          }
          else if (command == "lockTurnstile") {
            handleLockTurnstile(client);
          }
          else if (command == "reboot") {
            handleReboot(client);
          }
        }
      }
    }
  }
}

void WebServerManager::handleGetConfig(AsyncWebSocketClient* client) {
  DynamicJsonDocument response(2048);
  response["type"] = "config";
  response["sensor_id"] = sensorId;
  response["firmware_version"] = firmwareVersion;
  response["wifi_connected"] = wifiManager->isConnected();
  response["wifi_ssid"] = wifiManager->getConnectedSSID();
  if (wifiManager->isConnected()) {
    response["wifi_ip"] = wifiManager->getLocalIP().toString();
  }
  
  JsonArray networks = response.createNestedArray("known_networks");
  for (const auto& wn : wifiManager->getKnownNetworks()) {
    JsonObject net = networks.createNestedObject();
    net["ssid"] = wn.ssid;
    net["has_password"] = (wn.password.length() > 0);
  }
  
  String json;
  serializeJson(response, json);
  client->text(json);
}

void WebServerManager::handleAddWiFi(AsyncWebSocketClient* client, 
                                     const String& ssid, const String& password) {
  // Validar longitud de SSID
  if (ssid.length() == 0 || ssid.length() > 32) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "SSID inválido. Debe tener entre 1 y 32 caracteres";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  // Validar longitud de password
  if (password.length() > 63) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Contraseña inválida. Máximo 63 caracteres";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  // Agregar o actualizar red
  wifiManager->addNetwork(ssid, password);
  configManager->save(wifiManager->getKnownNetworks());
  
  DynamicJsonDocument responseDoc(256);
  responseDoc["type"] = "success";
  responseDoc["message"] = "Red WiFi agregada. Use el botón 'Conectar' para intentar la conexión";
  String response;
  serializeJson(responseDoc, response);
  client->text(response);
  
  // No intentar conectar inmediatamente para evitar bloqueos
  // El usuario debe usar el botón "Conectar a Red Conocida"
}

void WebServerManager::handleDeleteWiFi(AsyncWebSocketClient* client, const String& ssid) {
  wifiManager->removeNetwork(ssid);
  configManager->save(wifiManager->getKnownNetworks());
  
  DynamicJsonDocument responseDoc(256);
  responseDoc["type"] = "success";
  responseDoc["message"] = "Red WiFi eliminada";
  String response;
  serializeJson(responseDoc, response);
  client->text(response);
}

void WebServerManager::handleScanNetworks(AsyncWebSocketClient* client) {
  Serial.println("Iniciando escaneo de redes WiFi...");
  
  if (wifiManager->isScanInProgress()) {
    DynamicJsonDocument responseDoc(256);
    responseDoc["type"] = "scan_started";
    responseDoc["message"] = "Escaneo ya en progreso...";
    String response;
    serializeJson(responseDoc, response);
    client->text(response);
    return;
  }
  
  bool started = wifiManager->startScan();
  
  if (started) {
    DynamicJsonDocument responseDoc(256);
    responseDoc["type"] = "scan_started";
    responseDoc["message"] = "Escaneo iniciado, espere unos segundos...";
    String response;
    serializeJson(responseDoc, response);
    client->text(response);
  } else {
    DynamicJsonDocument responseDoc(256);
    responseDoc["type"] = "error";
    responseDoc["message"] = "Error al escanear redes. Intente nuevamente";
    String response;
    serializeJson(responseDoc, response);
    client->text(response);
  }
}

void WebServerManager::handleConnectWiFi(AsyncWebSocketClient* client) {
  wifiManager->tryConnectKnownNetworks();
}

// ============================================================================
// MODO HANDLERS
// ============================================================================
void WebServerManager::handleSetMode(AsyncWebSocketClient* client, const String& mode) {
  if (!modeManager) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Mode manager no inicializado";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  SensorMode newMode = stringToMode(mode);
  
  if (modeManager->setMode(newMode)) {
    DynamicJsonDocument responseDoc(512);
    responseDoc["type"] = "success";
    responseDoc["message"] = "Modo cambiado a: " + mode + ". Reinicie el dispositivo para aplicar cambios.";
    responseDoc["current_mode"] = modeToString(newMode);
    responseDoc["requires_reboot"] = true;
    String response;
    serializeJson(responseDoc, response);
    client->text(response);
    
    // Notificar a todos los clientes
    notifyClients(response);
  } else {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Error al cambiar modo";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
  }
}

void WebServerManager::handleGetMode(AsyncWebSocketClient* client) {
  if (!modeManager) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Mode manager no inicializado";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  DynamicJsonDocument responseDoc(256);
  responseDoc["type"] = "mode_info";
  responseDoc["current_mode"] = modeManager->getCurrentModeString();
  String response;
  serializeJson(responseDoc, response);
  client->text(response);
}

// ============================================================================
// TORNIQUETE HANDLERS
// ============================================================================
void WebServerManager::handleAddAuthorizedCard(AsyncWebSocketClient* client, const String& uid) {
  if (!turnstileMode) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Modo torniquete no activo";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  turnstileMode->addAuthorizedCard(uid);
  
  DynamicJsonDocument responseDoc(256);
  responseDoc["type"] = "success";
  responseDoc["message"] = "Tarjeta " + uid + " agregada";
  String response;
  serializeJson(responseDoc, response);
  client->text(response);
}

void WebServerManager::handleRemoveAuthorizedCard(AsyncWebSocketClient* client, const String& uid) {
  if (!turnstileMode) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Modo torniquete no activo";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  turnstileMode->removeAuthorizedCard(uid);
  
  DynamicJsonDocument responseDoc(256);
  responseDoc["type"] = "success";
  responseDoc["message"] = "Tarjeta " + uid + " eliminada";
  String response;
  serializeJson(responseDoc, response);
  client->text(response);
}

void WebServerManager::handleGetAuthorizedCards(AsyncWebSocketClient* client) {
  if (!turnstileMode) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Modo torniquete no activo";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  auto cards = turnstileMode->getAuthorizedCards();
  
  DynamicJsonDocument responseDoc(1024);
  responseDoc["type"] = "authorized_cards";
  JsonArray cardsArray = responseDoc.createNestedArray("cards");
  for (const auto& uid : cards) {
    cardsArray.add(uid);
  }
  
  String response;
  serializeJson(responseDoc, response);
  client->text(response);
}

void WebServerManager::handleUnlockTurnstile(AsyncWebSocketClient* client, uint16_t duration) {
  if (!turnstileMode) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Modo torniquete no activo";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  bool success = false;
  String message;
  
  if (duration == 0) {
    // Usar el delay configurado por defecto
    success = turnstileMode->unlockTurnstile();
    unsigned long delayMs = turnstileMode->getAutoLockDelay();
    if (delayMs == 0) {
      message = "Torniquete desbloqueado INDEFINIDAMENTE";
    } else {
      message = "Torniquete desbloqueado por " + String(delayMs / 1000) + " segundos";
    }
  } else if (duration == 9999) {
    // Valor especial para indefinido
    success = turnstileMode->unlockTurnstile(0);
    message = "Torniquete desbloqueado INDEFINIDAMENTE";
  } else {
    // Duración específica
    success = turnstileMode->unlockTurnstile(duration);
    message = "Torniquete desbloqueado por " + String(duration) + " segundos";
  }
  
  if (success) {
    DynamicJsonDocument responseDoc(256);
    responseDoc["type"] = "success";
    responseDoc["message"] = message;
    responseDoc["duration"] = duration;
    String response;
    serializeJson(responseDoc, response);
    client->text(response);
  } else {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Error al desbloquear torniquete";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
  }
}

void WebServerManager::handleLockTurnstile(AsyncWebSocketClient* client) {
  if (!turnstileMode) {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Modo torniquete no activo";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
    return;
  }
  
  if (turnstileMode->lockTurnstile()) {
    DynamicJsonDocument responseDoc(256);
    responseDoc["type"] = "success";
    responseDoc["message"] = "Torniquete bloqueado";
    String response;
    serializeJson(responseDoc, response);
    client->text(response);
  } else {
    DynamicJsonDocument errorDoc(256);
    errorDoc["type"] = "error";
    errorDoc["message"] = "Error al bloquear torniquete";
    String error;
    serializeJson(errorDoc, error);
    client->text(error);
  }
}

// ============================================================================
// SISTEMA HANDLERS
// ============================================================================
void WebServerManager::handleReboot(AsyncWebSocketClient* client) {
  DynamicJsonDocument responseDoc(256);
  responseDoc["type"] = "success";
  responseDoc["message"] = "Reiniciando dispositivo en 2 segundos...";
  String response;
  serializeJson(responseDoc, response);
  client->text(response);
  
  // Notificar a todos los clientes
  notifyClients(response);
  
  // Esperar un momento para que se envíe el mensaje
  delay(1000);
  
  // Reiniciar el ESP32 usando power.h
  restart(0);
}
