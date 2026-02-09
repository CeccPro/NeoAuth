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
  
  // Leer archivo JSON
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("[ConfigManager] ERROR: Failed to parse config file");
    return false;
  }
  
  // Cargar modo
  if (doc.containsKey("mode")) {
    mode = doc["mode"].as<String>();
  }
  
  // Cargar redes WiFi
  if (doc.containsKey("wifi_networks")) {
    JsonArray wifiNetworks = doc["wifi_networks"].as<JsonArray>();
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
  return true;
}

String ConfigManager::getAPIBaseURL() {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) return "";
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error || !doc.containsKey("api")) return "";
  
  return doc["api"]["base_url"] | "";
}

bool ConfigManager::getAPIEnabled() {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) return false;
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error || !doc.containsKey("api")) return false;
  
  return doc["api"]["enabled"] | false;
}

unsigned long ConfigManager::getAPIHeartbeatInterval() {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) return 300000; // Default: 5 minutos
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error || !doc.containsKey("api")) return 300000;
  
  return (doc["api"]["heartbeat_interval"] | 300) * 1000; // Convertir segundos a ms
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
  
  // Actualizar configuración de API
  JsonObject api = doc.createNestedObject("api");
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
