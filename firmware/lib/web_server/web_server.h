/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: web_server.h
 * Author: CeccPro
 * 
 * Description:
 *   Web Server Manager - Handles HTTP server and WebSocket communication
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Forward declarations
class WiFiManager;
class ConfigManager;
class ModeManager;
class TurnstileMode;

// Callback types
typedef std::function<void(const String& ssid, const String& password)> OnAddWiFiCallback;
typedef std::function<void(const String& ssid)> OnDeleteWiFiCallback;
typedef std::function<void()> OnScanNetworksCallback;
typedef std::function<void()> OnConnectWiFiCallback;

class WebServerManager {
public:
  WebServerManager(uint16_t port = 80);
  
  // Inicialización
  void begin(WiFiManager* wifiMgr, ConfigManager* configMgr,
             const char* sensorId, const char* firmwareVersion);
  
  // Agregar referencias al modo manager y turnstile
  void setModeManager(ModeManager* modeMgr);
  void setTurnstileMode(TurnstileMode* turnstileModePtr);
  
  // Notificar a todos los clientes WebSocket
  void notifyClients(const String& message);
  
  // Enviar información del sistema
  void sendSystemInfo(AsyncWebSocketClient* client = nullptr);
  
  // Enviar métricas del sistema (RAM, CPU, almacenamiento)
  void sendSystemMetrics(AsyncWebSocketClient* client = nullptr);
  
  // Enviar hora del RTC
  void sendRTCTime(AsyncWebSocketClient* client = nullptr);
  
  // Enviar resultado del escaneo WiFi
  void sendScanResult(int networkCount);
  
  // Enviar notificación de tarjeta RFID
  void notifyCardDetected(const String& uid);
  void notifyCardDetected(const String& uid, bool accessGranted, const String& userName, JsonObject userMetadata);
  
  // Enviar notificación de conexión WiFi
  void notifyWiFiConnected(const String& ssid, const String& ip);
  void notifyWiFiDisconnected();
  
  // Tareas periódicas (llamar en loop)
  void periodicTask();

private:
  AsyncWebServer server;
  ModeManager* modeManager;
  TurnstileMode* turnstileMode;
  AsyncWebSocket ws;
  
  WiFiManager* wifiManager;
  ConfigManager* configManager;
  const char* sensorId;
  const char* firmwareVersion;
  
  // Manejadores de eventos WebSocket
  void handleWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                           AwsEventType type, void *arg, uint8_t *data, size_t len);
  
  // Manejadores de comandos
  void handleGetConfig(AsyncWebSocketClient* client);
  void handleAddWiFi(AsyncWebSocketClient* client, const String& ssid, const String& password, bool preferred = false);
  void handleSetPreferredWiFi(AsyncWebSocketClient* client, const String& ssid);
  void handleConnectToWiFi(AsyncWebSocketClient* client, const String& ssid);
  void handleDeleteWiFi(AsyncWebSocketClient* client, const String& ssid);
  
  // Manejadores de modo
  void handleSetMode(AsyncWebSocketClient* client, const String& mode);
  void handleGetMode(AsyncWebSocketClient* client);
  
  // Manejadores de torniquete
  void handleAddAuthorizedCard(AsyncWebSocketClient* client, const String& uid);
  void handleRemoveAuthorizedCard(AsyncWebSocketClient* client, const String& uid);
  void handleGetAuthorizedCards(AsyncWebSocketClient* client);
  void handleUnlockTurnstile(AsyncWebSocketClient* client, JsonVariant params);
  void handleLockTurnstile(AsyncWebSocketClient* client);
  void handleSetBlockMode(AsyncWebSocketClient* client, bool enabled);
  void handleScanNetworks(AsyncWebSocketClient* client);
  void handleConnectWiFi(AsyncWebSocketClient* client);
  
  // Manejadores de configuración
  void handleUpdateConfig(AsyncWebSocketClient* client, JsonVariant config);
  void handleResetConfig(AsyncWebSocketClient* client);
  void handleReboot(AsyncWebSocketClient* client);
  
  // Manejadores de métricas
  void handleGetSystemMetrics(AsyncWebSocketClient* client);
};

#endif // WEB_SERVER_H
