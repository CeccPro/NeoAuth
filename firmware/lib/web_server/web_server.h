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
class TimeManager;

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
  void setTimeManager(TimeManager* timeMgr);
  
  // Notificar a todos los clientes WebSocket
  void notifyClients(const String& message);
  
  // Enviar información del sistema
  void sendSystemInfo(AsyncWebSocketClient* client = nullptr);
  void sendSystemMetrics(AsyncWebSocketClient* client = nullptr);
  void sendTimeInfo(AsyncWebSocketClient* client = nullptr);
  
  // Enviar resultado del escaneo WiFi
  void sendScanResult(int networkCount);
  
  // Enviar notificación de tarjeta RFID
  void notifyCardDetected(const String& uid);
  
  // Enviar notificación de conexión WiFi
  void notifyWiFiConnected(const String& ssid, const String& ip);
  void notifyWiFiDisconnected();
  
  // Enviar notificación de evento de acceso (torniquete)
  void notifyAccessEvent(const String& uid, bool granted);
  
  // Enviar notificación de identificación (standalone)
  void notifyIdentificationEvent(const String& uid, bool found, const String& userName, const String& userEmail);
  
  // Tareas periódicas (llamar en loop)
  void periodicTask();

private:
  AsyncWebServer server;
  ModeManager* modeManager;
  TurnstileMode* turnstileMode;
  TimeManager* timeManager;
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
  void handleGetSystemMetrics(AsyncWebSocketClient* client);
  void handleAddWiFi(AsyncWebSocketClient* client, const String& ssid, const String& password);
  void handleDeleteWiFi(AsyncWebSocketClient* client, const String& ssid);
  
  // Manejadores de modo
  void handleSetMode(AsyncWebSocketClient* client, const String& mode);
  void handleGetMode(AsyncWebSocketClient* client);
  
  // Manejadores de torniquete
  void handleAddAuthorizedCard(AsyncWebSocketClient* client, const String& uid);
  void handleRemoveAuthorizedCard(AsyncWebSocketClient* client, const String& uid);
  void handleGetAuthorizedCards(AsyncWebSocketClient* client);
  void handleUnlockTurnstile(AsyncWebSocketClient* client, uint16_t duration = 0);
  void handleLockTurnstile(AsyncWebSocketClient* client);
  void handleScanNetworks(AsyncWebSocketClient* client);
  void handleConnectWiFi(AsyncWebSocketClient* client);
  
  // Manejadores del sistema
  void handleReboot(AsyncWebSocketClient* client);
};

#endif // WEB_SERVER_H
