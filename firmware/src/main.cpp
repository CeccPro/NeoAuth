/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: main.cpp
 * Author: CeccPro
 * 
 * Description:
 *   Main application - RFID Auth Project
 *   Modular architecture with task-based loop for watchdog safety
 */

#include <Arduino.h>
#include <power.h>
#include <config.h>
#include <wifi_manager.h>
#include <config_manager.h>
#include <rfid_manager.h>
#include <web_server.h>

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
WiFiManager wifiManager(AP_SSID, AP_PASSWORD);
ConfigManager configManager(CONFIG_FILE_PATH);
RFIDManager rfidManager(SS_PIN, RST_PIN);
WebServerManager webServer(WEB_SERVER_PORT);

// ============================================================================
// TASK MANAGEMENT (para evitar bloqueos del watchdog)
// ============================================================================
enum TaskType {
  TASK_WIFI_PERIODIC,
  TASK_WIFI_CHECK_CONNECTION,
  TASK_WIFI_CHECK_SCAN,
  TASK_RFID_CHECK_CARD,
  TASK_WEBSERVER_CLEANUP,
  TASK_COUNT
};

struct Task {
  TaskType type;
  unsigned long lastRun;
  unsigned long interval;
  bool enabled;
};

Task tasks[TASK_COUNT] = {
  {TASK_WIFI_PERIODIC,        0, 30000, true},  // Cada 30 segundos
  {TASK_WIFI_CHECK_CONNECTION, 0, 5000,  true},  // Cada 5 segundos
  {TASK_WIFI_CHECK_SCAN,       0, 100,   true},  // Cada 100ms
  {TASK_RFID_CHECK_CARD,       0, 50,    true},  // Cada 50ms
  {TASK_WEBSERVER_CLEANUP,     0, 2000,  true}   // Cada 2 segundos
};

// ============================================================================
// TASK HANDLERS
// ============================================================================
void handleWiFiPeriodicTask() {
  wifiManager.periodicTask();
}

void handleWiFiCheckConnection() {
  bool wasConnected = wifiManager.isConnected();
  wifiManager.checkConnectionStatus();
  
  // Si perdimos la conexión, notificar
  if (wasConnected && !wifiManager.isConnected()) {
    webServer.notifyWiFiDisconnected();
  }
}

void handleWiFiCheckScan() {
  if (wifiManager.isScanInProgress()) {
    int result = wifiManager.checkScanComplete();
    if (result >= 0) {
      // Escaneo completado, enviar resultados
      webServer.sendScanResult(result);
      wifiManager.markScanResultReported();
    } else if (result == -1) {
      // Escaneo falló - solo reportar una vez
      if (!wifiManager.isScanResultReported()) {
        webServer.sendScanResult(-1);
        wifiManager.markScanResultReported();
      }
    }
    // Si result == WIFI_SCAN_RUNNING o -3 (reintento), no hacer nada
  }
}

void handleRFIDCheckCard() {
  String uid;
  if (rfidManager.checkForCard(uid)) {
    Serial.println("Tarjeta detectada: " + uid);
    webServer.notifyCardDetected(uid);
  }
}

void handleWebServerCleanup() {
  webServer.periodicTask();
}

// ============================================================================
// TASK SCHEDULER
// ============================================================================
void runTasks() {
  unsigned long currentMillis = millis();
  
  for (int i = 0; i < TASK_COUNT; i++) {
    Task& task = tasks[i];
    
    if (!task.enabled) {
      continue;
    }
    
    // Verificar si es hora de ejecutar esta tarea
    // Protección contra overflow de millis()
    if (currentMillis - task.lastRun >= task.interval || currentMillis < task.lastRun) {
      task.lastRun = currentMillis;
      
      // Ejecutar la tarea correspondiente
      switch (task.type) {
        case TASK_WIFI_PERIODIC:
          handleWiFiPeriodicTask();
          break;
        case TASK_WIFI_CHECK_CONNECTION:
          handleWiFiCheckConnection();
          break;
        case TASK_WIFI_CHECK_SCAN:
          handleWiFiCheckScan();
          break;
        case TASK_RFID_CHECK_CARD:
          handleRFIDCheckCard();
          break;
        case TASK_WEBSERVER_CLEANUP:
          handleWebServerCleanup();
          break;
        default:
          break;
      }
      
      // Yield después de cada tarea para alimentar el watchdog
      yield();
    }
  }
}


// ============================================================================
// SETUP
// ============================================================================
void setup() {
  // Inicializar comunicación serial
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(SERIAL_READ_TIMEOUT);
  delay(500); // Estabilizar comunicación serial

  Serial.println("===========================================");
  Serial.println("RFID Auth Project");
  Serial.println("Sensor ID: " + String(RFID_SENSOR_ID));
  Serial.println("Firmware: " + String(FIRMWARE_VERSION));
  Serial.println("===========================================");

  // Inicializar ConfigManager (SPIFFS)
  if (!configManager.begin()) {
    Serial.println("ERROR: Failed to initialize SPIFFS");
    poweroff(2);
    for(;;);
  }
  
  // Cargar configuración
  std::vector<WiFiNetwork> networks;
  if (configManager.load(networks)) {
    wifiManager.setKnownNetworks(networks);
  }
  
  // Inicializar RFID Manager
  if (!rfidManager.begin()) {
    Serial.println("ERROR: RFID sensor initialization failed");
    poweroff(3);
    for (;;);
  }

  // Inicializar WiFi Manager
  wifiManager.begin();
  
  // Inicializar Web Server
  webServer.begin(&wifiManager, &configManager, RFID_SENSOR_ID, FIRMWARE_VERSION);
  
  // Configurar callback para notificar conexiones WiFi exitosas
  wifiManager.setOnConnectedCallback([](const String& ssid, const String& ip) {
    Serial.println("Callback: WiFi conectado a " + ssid + " con IP " + ip);
    webServer.notifyWiFiConnected(ssid, ip);
  });
  
  // Intentar conectar a redes WiFi conocidas
  if (!wifiManager.getKnownNetworks().empty()) {
    Serial.println("Intentando conectar a redes WiFi conocidas...");
    if (wifiManager.tryConnectKnownNetworks()) {
      // Notificar conexión exitosa
      webServer.notifyWiFiConnected(
        wifiManager.getConnectedSSID(),
        wifiManager.getLocalIP().toString()
      );
    }
  }
  Serial.println("AP SSID: " + String(AP_SSID));
  // Inicializar Web Server
  webServer.begin(&wifiManager, &configManager, RFID_SENSOR_ID, FIRMWARE_VERSION);
  if (wifiManager.isConnected()) {
    Serial.println("WiFi conectado a: " + wifiManager.getConnectedSSID());
    Serial.println("WiFi IP: " + wifiManager.getLocalIP().toString());
  }  
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  // Ejecutar todas las tareas de forma no bloqueante
  runTasks();
  
  // Pequeño delay para no saturar el CPU
  delay(5);
  
  // Yield adicional para alimentar el watchdog
  yield();
}

