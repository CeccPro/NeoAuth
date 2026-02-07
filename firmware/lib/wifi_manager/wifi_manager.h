/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: wifi_manager.h
 * Author: CeccPro
 * 
 * Description:
 *   WiFi Manager - Manages WiFi connections, network scanning, and Access Point
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

// Callback para notificar conexión exitosa
typedef std::function<void(const String& ssid, const String& ip)> OnWiFiConnectedCallback;

// Estructura para almacenar redes WiFi conocidas
struct WiFiNetwork {
  String ssid;
  String password;
  bool preferred = false; // Red preferida para auto-conexión
};

class WiFiManager {
public:
  WiFiManager(const char* ap_ssid, const char* ap_password);
  
  // Inicialización
  void begin();
  
  // Gestión de redes conocidas
  void addNetwork(const String& ssid, const String& password, bool preferred = false);
  bool removeNetwork(const String& ssid);
  bool networkExists(const String& ssid);
  void updateNetworkPassword(const String& ssid, const String& password);
  void setPreferredNetwork(const String& ssid);
  const std::vector<WiFiNetwork>& getKnownNetworks() const;
  void setKnownNetworks(const std::vector<WiFiNetwork>& networks);
  
  // Conexión
  bool tryConnectKnownNetworks();
  bool isConnected() const { return wifiConnected; }
  String getConnectedSSID() const { return currentConnectedSSID; }
  IPAddress getLocalIP() const { return WiFi.localIP(); }
  IPAddress getAPIP() const { return WiFi.softAPIP(); }
  
  // Escaneo de redes
  bool startScan();
  bool isScanInProgress() const { return scanInProgress; }
  int checkScanComplete(); // Retorna número de redes o código de estado
  bool isScanResultReported() const { return scanResultReported; }
  void markScanResultReported() { scanResultReported = true; }
  
  // Tareas periódicas (llamar en loop)
  void periodicTask();
  
  // Callback para notificar conexiones
  void setOnConnectedCallback(OnWiFiConnectedCallback callback) { onConnectedCallback = callback; }
  
  // Estado
  void checkConnectionStatus();
  void resetConnectionState();

private:
  const char* ap_ssid;
  const char* ap_password;
  
  std::vector<WiFiNetwork> knownNetworks;
  String currentConnectedSSID;
  bool wifiConnected;
  unsigned long lastWiFiScan;
  bool scanInProgress;
  bool scanResultReported;
  int scanRetries; // Contador de reintentos de escaneo
  OnWiFiConnectedCallback onConnectedCallback;
  
  static const unsigned long WIFI_SCAN_INTERVAL = 30000; // 30 segundos
  static const int MAX_SCAN_RETRIES = 3; // Máximo número de reintentos
};

#endif // WIFI_MANAGER_H
