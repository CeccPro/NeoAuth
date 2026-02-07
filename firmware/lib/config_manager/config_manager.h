/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: config_manager.h
 * Author: CeccPro
 * 
 * Description:
 *   Configuration Manager - Handles loading and saving configuration to SPIFFS
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <vector>

// Forward declaration
struct WiFiNetwork;

class ConfigManager {
public:
  ConfigManager(const char* configFilePath = "/config.json");
  
  // Inicialización
  bool begin();
  
  // Carga y guarda configuración
  bool load(std::vector<WiFiNetwork>& networks);
  bool save(const std::vector<WiFiNetwork>& networks);
  
  // Gestión del modo
  String getMode();
  bool setMode(const String& mode);
  
  // Gestión de configuración de API
  String getAPIBaseURL();
  bool getAPIEnabled();
  unsigned long getAPIHeartbeatInterval();
  bool setAPIConfig(const String& baseURL, bool enabled, unsigned long heartbeatInterval);
  
  // Actualización de configuraciones individuales
  bool setHeartbeatInterval(unsigned long intervalMs);
  bool setAutoLockDelay(unsigned long delayMs);
  bool saveConfig(); // Guardar archivo config.json completo
  bool resetToDefaults(); // Restablecer configuración de fábrica

private:
  const char* configFilePath;
  bool createDefaultConfig();
  
  // Cache de configuración en memoria
  DynamicJsonDocument* configCache;
  bool loadConfigToCache();
};

#endif // CONFIG_MANAGER_H
