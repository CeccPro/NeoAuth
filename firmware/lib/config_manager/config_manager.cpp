/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: config_manager.cpp
 * Author: CeccPro
 * 
 * Description:
 *   Configuration Manager Implementation
 */

#include "config_manager.h"
#include "../wifi_manager/wifi_manager.h"

ConfigManager::ConfigManager(const char* configFilePath)
  : configFilePath(configFilePath), configCache(nullptr) {
}

bool ConfigManager::begin() {
  Serial.println("Initializing SPIFFS");
  if(!SPIFFS.begin(true)) {  // true = formatea si falla
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  
  // Inicializar cache
  configCache = new DynamicJsonDocument(2048);
  
  return true;
}

bool ConfigManager::load(std::vector<WiFiNetwork>& networks, String& mode) {
  networks.clear();
  mode = "";
  
  if (!SPIFFS.exists(configFilePath)) {
    Serial.println("[ConfigManager] Config file not found");
    return false;
  }
  
  File file = SPIFFS.open(configFilePath, FILE_READ);
  if (!file) {
    Serial.println("[ConfigManager] ERROR: Failed to open config file");
    return false;
  }
  
  // Leer archivo JSON y guardar en cache
  DeserializationError error = deserializeJson(*configCache, file);
  file.close();
  
  if (error) {
    Serial.println("[ConfigManager] ERROR: Failed to parse config file");
    return false;
  }
  
  // Cargar modo
  if (configCache->containsKey("mode")) {
    mode = (*configCache)["mode"].as<String>();
  }
  
  // Cargar redes WiFi
  if (configCache->containsKey("wifi_networks")) {
    JsonArray wifiNetworks = (*configCache)["wifi_networks"].as<JsonArray>();
    for (JsonObject net : wifiNetworks) {
      WiFiNetwork wn;
      wn.ssid = net["ssid"].as<String>();
      wn.password = net["password"].as<String>();
      networks.push_back(wn);
    }
  }
  
  Serial.println("[ConfigManager] Config loaded successfully");
  Serial.println("[ConfigManager] Mode: " + mode);
  Serial.println("[ConfigManager] Networks: " + String(networks.size()));
  
  // Debug API config
  if (configCache->containsKey("api")) {
    String apiUrl = (*configCache)["api"]["base_url"] | "";
    bool apiEnabled = (*configCache)["api"]["enabled"] | false;
    Serial.println("[ConfigManager] API Base URL: " + apiUrl);
    Serial.println("[ConfigManager] API Enabled: " + String(apiEnabled ? "YES" : "NO"));
  } else {
    Serial.println("[ConfigManager] No API configuration found in config.json");
  }
  
  return true;
}

bool ConfigManager::save(const std::vector<WiFiNetwork>& networks, const String& mode) {
  DynamicJsonDocument doc(2048);
  
  JsonArray networksArray = doc.createNestedArray("wifi_networks");
  for (const auto& wn : networks) {
    JsonObject network = networksArray.createNestedObject();
    network["ssid"] = wn.ssid;
    network["password"] = wn.password;
    network["preferred"] = wn.preferred;
  }

  doc["mode"] = mode;

  File configFile = SPIFFS.open(configFilePath, "w");
  if (!configFile) {
    Serial.println("Error abriendo config.json para escritura");
    return false;
  }

  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Error escribiendo config.json");
    configFile.close();
    return false;
  }

  configFile.close();
  Serial.println("Configuración guardada correctamente");
  return true;
}

bool ConfigManager::createDefaultConfig() {
  std::vector<WiFiNetwork> emptyNetworks;
  
  return save(emptyNetworks, "standalone");
}

String ConfigManager::getMode() {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) {
    Serial.println("[ConfigManager] Error opening config.json");
    return "standalone"; // Default
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("[ConfigManager] Error parsing config.json");
    return "standalone"; // Default
  }
  
  if (!doc.containsKey("mode")) {
    return "standalone"; // Default
  }
  
  return doc["mode"].as<String>();
}

bool ConfigManager::setMode(const String& mode) {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) {
    Serial.println("[ConfigManager] Error opening config.json");
    return false;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("[ConfigManager] Error parsing config.json");
    return false;
  }
  
  // Actualizar el modo
  doc["mode"] = mode;
  
  // Guardar
  configFile = SPIFFS.open(configFilePath, "w");
  if (!configFile) {
    Serial.println("[ConfigManager] Error opening config.json for writing");
    return false;
  }
  
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("[ConfigManager] Error writing config.json");
    configFile.close();
    return false;
  }

  configFile.close();
  
  // Actualizar cache con la nueva configuración
  configCache = std::make_unique<DynamicJsonDocument>(2048);
  *configCache = doc;
  
  return true;
}

String ConfigManager::getAPIBaseURL() {
  if (!configCache || configCache->isNull()) {
    Serial.println("[ConfigManager] getAPIBaseURL: Config cache is empty");
    return "";
  }
  
  if (!configCache->containsKey("api")) {
    Serial.println("[ConfigManager] getAPIBaseURL: No 'api' key in cache");
    return "";
  }
  
  String baseUrl = (*configCache)["api"]["base_url"] | "";
  Serial.println("[ConfigManager] getAPIBaseURL: " + baseUrl);
  return baseUrl;
}

bool ConfigManager::getAPIEnabled() {
  if (!configCache || configCache->isNull()) {
    Serial.println("[ConfigManager] getAPIEnabled: Config cache is empty");
    return false;
  }
  
  if (!configCache->containsKey("api")) {
    Serial.println("[ConfigManager] getAPIEnabled: No 'api' key in cache");
    return false;
  }
  
  bool enabled = (*configCache)["api"]["enabled"] | false;
  Serial.println("[ConfigManager] getAPIEnabled: " + String(enabled ? "true" : "false"));
  return enabled;
}

unsigned long ConfigManager::getAPIHeartbeatInterval() {
  if (!configCache || configCache->isNull() || !configCache->containsKey("api")) {
    return 300000; // Default: 5 minutos
  }
  
  return ((*configCache)["api"]["heartbeat_interval"] | 300) * 1000; // Convertir segundos a ms
}

bool ConfigManager::setAPIConfig(const String& baseURL, bool enabled, unsigned long heartbeatInterval) {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) {
    Serial.println("[ConfigManager] Error opening config.json");
    return false;
  }
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("[ConfigManager] Error parsing config.json");
    return false;
  }
  
  // Actualizar configuración de API (fusionar, no sobrescribir)
  if (!doc.containsKey("api")) {
    doc.createNestedObject("api");
  }
  
  JsonObject api = doc["api"];
  api["base_url"] = baseURL;
  api["enabled"] = enabled;
  api["heartbeat_interval"] = heartbeatInterval / 1000; // Guardar en segundos
  
  // Guardar
  configFile = SPIFFS.open(configFilePath, "w");
  if (!configFile) {
    Serial.println("[ConfigManager] Error opening config.json for writing");
    return false;
  }
  
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("[ConfigManager] Error writing config.json");
    configFile.close();
    return false;
  }
  
  configFile.close();
  return true;
}

bool ConfigManager::loadConfigToCache() {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) {
    Serial.println("[ConfigManager] Error opening config.json");
    return false;
  }
  
  DeserializationError error = deserializeJson(*configCache, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("[ConfigManager] Error parsing config.json");
    return false;
  }
  
  return true;
}

bool ConfigManager::saveConfig() {
  File configFile = SPIFFS.open(configFilePath, "w");
  if (!configFile) {
    Serial.println("[ConfigManager] Error opening config.json for writing");
    return false;
  }
  
  if (serializeJson(*configCache, configFile) == 0) {
    Serial.println("[ConfigManager] Error writing config.json");
    configFile.close();
    return false;
  }
  
  configFile.close();
  Serial.println("[ConfigManager] Configuration saved");
  return true;
}

bool ConfigManager::setHeartbeatInterval(unsigned long intervalMs) {
  if (!loadConfigToCache()) return false;
  
  // Actualizar en cache
  if (!(*configCache)["api"].is<JsonObject>()) {
    (*configCache).createNestedObject("api");
  }
  
  (*configCache)["api"]["heartbeat_interval"] = intervalMs / 1000; // Guardar en segundos
  
  return true; // No guardar todavía, se guardará con saveConfig()
}

bool ConfigManager::setAutoLockDelay(unsigned long delayMs) {
  if (!loadConfigToCache()) return false;
  
  // Actualizar en cache
  if (!(*configCache)["turnstile"].is<JsonObject>()) {
    (*configCache).createNestedObject("turnstile");
  }
  
  (*configCache)["turnstile"]["auto_lock_delay"] = delayMs;
  
  return true; // No guardar todavía, se guardará con saveConfig()
}

bool ConfigManager::resetToDefaults() {
  // Crear configuración vacía por defecto
  configCache->clear();
  (*configCache)["mode"] = "standalone";
  
  JsonArray networks = configCache->createNestedArray("wifi_networks");
  
  JsonObject api = configCache->createNestedObject("api");
  api["base_url"] = "";
  api["enabled"] = false;
  api["heartbeat_interval"] = 300;
  
  JsonObject turnstile = configCache->createNestedObject("turnstile");
  turnstile["auto_lock_delay"] = 5000;
  
  return saveConfig();
}
