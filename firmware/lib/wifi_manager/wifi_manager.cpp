/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: wifi_manager.cpp
 * Author: CeccPro
 * 
 * Description:
 *   WiFi Manager Implementation
 */

#include "wifi_manager.h"

WiFiManager::WiFiManager(const char* ap_ssid, const char* ap_password)
  : ap_ssid(ap_ssid), ap_password(ap_password), 
    wifiConnected(false), lastWiFiScan(0), scanInProgress(false), scanResultReported(false), scanRetries(0),
    onConnectedCallback(nullptr) {
}

void WiFiManager::begin() {
  // Inicializa AP en modo AP_STA desde el inicio para permitir escaneos
  WiFi.mode(WIFI_AP_STA);
  
  // Verificar que el AP se inicie correctamente
  bool apStarted = WiFi.softAP(ap_ssid, ap_password);
  if (apStarted) {
    Serial.println("AP iniciado correctamente");
    Serial.print("IP del AP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("ERROR: No se pudo iniciar el AP");
  }
}

void WiFiManager::addNetwork(const String& ssid, const String& password, bool preferred) {
  // Verificar si ya existe y actualizar
  for (auto& wn : knownNetworks) {
    if (wn.ssid == ssid) {
      wn.password = password;
      if (preferred) {
        wn.preferred = true;
        // Quitar preferred de otras redes
        for (auto& other : knownNetworks) {
          if (other.ssid != ssid) other.preferred = false;
        }
      }
      return;
    }
  }
  
  // Si no existe, agregar nueva red
  WiFiNetwork wn;
  wn.ssid = ssid;
  wn.password = password;
  wn.preferred = preferred;
  
  // Si es preferida, quitar preferred de otras redes
  if (preferred) {
    for (auto& other : knownNetworks) {
      other.preferred = false;
    }
  }
  
  knownNetworks.push_back(wn);
}

void WiFiManager::setPreferredNetwork(const String& ssid) {
  for (auto& wn : knownNetworks) {
    wn.preferred = (wn.ssid == ssid);
  }
}

bool WiFiManager::removeNetwork(const String& ssid) {
  for (auto it = knownNetworks.begin(); it != knownNetworks.end(); ++it) {
    if (it->ssid == ssid) {
      knownNetworks.erase(it);
      return true;
    }
  }
  return false;
}

bool WiFiManager::networkExists(const String& ssid) {
  for (const auto& wn : knownNetworks) {
    if (wn.ssid == ssid) {
      return true;
    }
  }
  return false;
}

void WiFiManager::updateNetworkPassword(const String& ssid, const String& password) {
  for (auto& wn : knownNetworks) {
    if (wn.ssid == ssid) {
      wn.password = password;
      return;
    }
  }
}

const std::vector<WiFiNetwork>& WiFiManager::getKnownNetworks() const {
  return knownNetworks;
}

void WiFiManager::setKnownNetworks(const std::vector<WiFiNetwork>& networks) {
  knownNetworks = networks;
}

bool WiFiManager::tryConnectKnownNetworks() {
  if (knownNetworks.empty()) {
    return false;
  }

  Serial.println("Escaneando redes WiFi disponibles...");
  
  // Usar escaneo síncrono con timeout corto
  int n = WiFi.scanNetworks(false, false, false, 300);
  
  if (n == WIFI_SCAN_RUNNING) {
    Serial.println("Escaneo en progreso, esperar...");
    return false;
  }
  
  Serial.println("Escaneo completado. Redes encontradas: " + String(n));

  for (int i = 0; i < n; i++) {
    String scannedSSID = WiFi.SSID(i);
    
    // Buscar si esta red está en nuestras redes conocidas
    for (const auto& known : knownNetworks) {
      if (scannedSSID == known.ssid) {
        Serial.println("Red conocida encontrada: " + scannedSSID);
        Serial.println("Intentando conectar...");
        
        // Limpiar cualquier conexión anterior
        WiFi.disconnect(true);
        delay(100);
        
        WiFi.mode(WIFI_AP_STA); // Modo AP + Estación
        WiFi.begin(known.ssid.c_str(), known.password.c_str());
        
        // Reducir timeout a 5 segundos (10 intentos x 500ms)
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 10) {
          delay(500);
          Serial.print(".");
          attempts++;
          yield(); // Alimentar el watchdog
        }
        yield(); // Yield adicional después del loop
        
        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true;
          currentConnectedSSID = scannedSSID;
          Serial.println("\n✓ Conectado a: " + scannedSSID);
          Serial.println("IP: " + WiFi.localIP().toString());
          return true;
        } else {
          Serial.println("\n✗ No se pudo conectar a: " + scannedSSID);
        }
      }
    }
  }
  
  Serial.println("No se encontraron redes WiFi conocidas disponibles");
  return false;
}

bool WiFiManager::startScan() {
  // Verificar estado actual
  int16_t scanStatus = WiFi.scanComplete();
  
  if (scanStatus == WIFI_SCAN_RUNNING) {
    Serial.println("Ya hay un escaneo en progreso");
    return false;
  }
  
  // Limpiar escaneos anteriores - SER AGRESIVO CON ESTO
  WiFi.scanDelete();
  delay(500); // Esperar más tiempo para que se limpie completamente
  
  // Reiniciar bandera de reporte para nuevo escaneo
  scanResultReported = false;
  
  // Iniciar escaneo asíncrono con parámetros más tolerantes
  // Parámetros: async=true, show_hidden=false, scan_type=0 (todos los canales), max_ms_per_chan=500
  Serial.println("Iniciando escaneo WiFi con timeout de 500ms por canal...");
  int16_t result = WiFi.scanNetworks(true, false, 0, 500);
  
  if (result == WIFI_SCAN_RUNNING) {
    scanInProgress = true;
    scanRetries = 0; // Resetear reintentos cuando inicia nuevo escaneo
    Serial.println("Escaneo asíncrono iniciado correctamente");
    return true;
  } else if (result >= 0) {
    // Escaneo completó instantáneamente
    Serial.printf("Escaneo completó rápidamente con %d redes\n", result);
    scanInProgress = false;
    scanResultReported = false; // Permitir reporte del resultado
    return true;
  }
  
  Serial.println("Error al iniciar escaneo - WiFi.scanNetworks() retornó: " + String(result));
  return false;
}

int WiFiManager::checkScanComplete() {
  if (!scanInProgress) {
    return -2; // No hay escaneo en progreso
  }
  
  int16_t n = WiFi.scanComplete();
  
  if (n >= 0) {
    scanInProgress = false;
    scanRetries = 0; // Resetear contador de reintentos
    Serial.println("Escaneo completado exitosamente. Redes encontradas: " + String(n));
    return n;
  } else if (n == WIFI_SCAN_FAILED) {
    scanInProgress = false;
    Serial.println("Escaneo falló - código de error: " + String(n));
    WiFi.scanDelete(); // Limpiar para próximo intento
    
    // Reintentar si no hemos alcanzado el máximo
    if (scanRetries < MAX_SCAN_RETRIES) {
      scanRetries++;
      Serial.println("Reintentando escaneo... (" + String(scanRetries) + "/" + String(MAX_SCAN_RETRIES) + ")");
      
      // Espera MUCHO más tiempo entre reintentos
      delay(1000);
      WiFi.scanDelete();
      delay(500);
      
      // Usar parámetros más robustos
      int16_t result = WiFi.scanNetworks(true, false, 0, 500);
      
      if (result == WIFI_SCAN_RUNNING) {
        scanInProgress = true;
        Serial.println("Escaneo asíncrono reiniciado en intento " + String(scanRetries));
        return WIFI_SCAN_RUNNING;
      }
      
      if (result >= 0) {
        Serial.printf("Escaneo completó en reintento con %d redes\n", result);
        scanInProgress = false;
        scanRetries = 0;
        return result;
      }
      
      // Si no pudo iniciar, esperar siguiente ciclo sin reportar error
      Serial.println("No se pudo iniciar escaneo en reintento, esperando siguiente ciclo...");
      return WIFI_SCAN_RUNNING; // Simular que aún está en progreso para reintentar después
    }
    
    scanRetries = 0; // Resetear para próximo intento manual
    scanResultReported = true; // Marcar como reportado para no reintentar inmediatamente
    Serial.println("Escaneo fallido después de " + String(MAX_SCAN_RETRIES) + " reintentos");
    return -1;
  }
  
  // Aún en progreso
  return WIFI_SCAN_RUNNING;
}

void WiFiManager::periodicTask() {
  // Escaneo periódico si no está conectado y hay redes conocidas
  if (!wifiConnected && !knownNetworks.empty()) {
    unsigned long currentMillis = millis();
    // Protección contra overflow de millis()
    if (currentMillis - lastWiFiScan >= WIFI_SCAN_INTERVAL || currentMillis < lastWiFiScan) {
      lastWiFiScan = currentMillis;
      Serial.println("Escaneo periódico de WiFi...");
      if (tryConnectKnownNetworks()) {
        // Notificar conexión exitosa si hay callback
        if (onConnectedCallback) {
          onConnectedCallback(currentConnectedSSID, WiFi.localIP().toString());
        }
      }
    }
  }
}

void WiFiManager::checkConnectionStatus() {
  // Verificar si perdimos la conexión WiFi
  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    currentConnectedSSID = "";
    Serial.println("Conexión WiFi perdida");
  }
}

void WiFiManager::resetConnectionState() {
  wifiConnected = false;
  currentConnectedSSID = "";
}
