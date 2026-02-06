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
  : configFilePath(configFilePath) {
}

bool ConfigManager::begin() {
  Serial.println("Initializing SPIFFS");
  if(!SPIFFS.begin(true)) {  // true = formatea si falla
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  return true;
}

bool ConfigManager::load(std::vector<WiFiNetwork>& networks) {
  File configFile = SPIFFS.open(configFilePath, "r");
  if (!configFile) {
    Serial.println("No existe config.json, creando configuración por defecto");
    createDefaultConfig();
    return true;
  }

  size_t size = configFile.size();
  if (size > 2048) {
    Serial.println("Config file demasiado grande");
    configFile.close();
    return false;
  }
  
  if (size == 0) {
    Serial.println("Config file vacío, creando configuración por defecto");
    configFile.close();
    createDefaultConfig();
    return true;
  }

  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, buf.get());
  
  if (error) {
    Serial.println("Error parseando config.json: " + String(error.c_str()));
    Serial.println("Creando configuración por defecto");
    createDefaultConfig();
    return true;
  }

  // Cargar redes WiFi conocidas
  networks.clear();
  JsonArray networksArray = doc["wifi_networks"];
  for (JsonObject network : networksArray) {
    WiFiNetwork wn;
    wn.ssid = network["ssid"].as<String>();
    wn.password = network["password"].as<String>();
    networks.push_back(wn);
    Serial.println("Red WiFi cargada: " + wn.ssid);
  }

  Serial.println("Configuración cargada correctamente");
  return true;
}

bool ConfigManager::save(const std::vector<WiFiNetwork>& networks) {
  DynamicJsonDocument doc(2048);
  
  JsonArray networksArray = doc.createNestedArray("wifi_networks");
  for (const auto& wn : networks) {
    JsonObject network = networksArray.createNestedObject();
    network["ssid"] = wn.ssid;
    network["password"] = wn.password;
  }

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
  return save(emptyNetworks);
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
