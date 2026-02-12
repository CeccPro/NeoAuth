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
#include <mode_manager.h>
#include <sideconn.h>
#include <turnstile_mode.h>
#include <api_client.h>
#include <standalone_mode.h>
#include <time_manager.h>

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
WiFiManager wifiManager(AP_SSID, AP_PASSWORD);
ConfigManager configManager(CONFIG_FILE_PATH);
RFIDManager rfidManager(SS_PIN, RST_PIN);
WebServerManager webServer(WEB_SERVER_PORT);
ModeManager modeManager;
APIClient apiClient(RFID_SENSOR_ID, AUTH_SECRET);
TimeManager timeManager("");
SideConn sideconn(SIDECONN_SDA_PIN, SIDECONN_SCL_PIN, SIDECONN_SLAVE_ADDR, MSG_4_BYTES);
TurnstileMode turnstileMode(&sideconn, &apiClient);
StandaloneMode standaloneMode(&apiClient);

// ============================================================================
// TASK MANAGEMENT (para evitar bloqueos del watchdog)
// ============================================================================
enum TaskType {
  TASK_WIFI_PERIODIC,
  TASK_WIFI_CHECK_CONNECTION,
  TASK_WIFI_CHECK_SCAN,
  TASK_RFID_CHECK_CARD,
  TASK_WEBSERVER_CLEANUP,
  TASK_TURNSTILE_PERIODIC,
  TASK_API_HEARTBEAT,
  TASK_TIME_SYNC,
  TASK_COUNT
};

struct Task {
  TaskType type;
  unsigned long lastRun;
  unsigned long interval;
  bool enabled;
};

Task tasks[TASK_COUNT] = {
  {TASK_WIFI_PERIODIC,        0, 30000,  true},  // Cada 30 segundos
  {TASK_WIFI_CHECK_CONNECTION, 0, 5000,   true},  // Cada 5 segundos
  {TASK_WIFI_CHECK_SCAN,       0, 100,    true},  // Cada 100ms
  {TASK_RFID_CHECK_CARD,       0, 50,     true},  // Cada 50ms
  {TASK_WEBSERVER_CLEANUP,     0, 2000,   true},  // Cada 2 segundos
  {TASK_TURNSTILE_PERIODIC,    0, 100,    false}, // Cada 100ms (solo en modo torniquete)
  {TASK_API_HEARTBEAT,         0, 300000, false}, // Cada 5 minutos (se activa si API habilitada)
  {TASK_TIME_SYNC,             0, 60000,  false}  // Cada 1 minuto (se activa si API habilitada)
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
    
    // Notificar detección al WebSocket
    webServer.notifyCardDetected(uid);
    
    // Procesar según el modo actual
    SensorMode currentMode = modeManager.getCurrentMode();
    
    if (currentMode == MODE_TURNSTILE) {
      turnstileMode.handleCardDetected(uid);
    } else if (currentMode == MODE_STANDALONE) {
      standaloneMode.handleCardDetected(uid);
    }
  }
}

void handleWebServerCleanup() {
  webServer.periodicTask();
}

void handleTurnstilePeriodicTask() {
  turnstileMode.periodicTask();
}

void handleAPIHeartbeatTask() {
  if (apiClient.isEnabled() && wifiManager.isConnected()) {
    Serial.println("[Heartbeat] Sending heartbeat to API...");
    
    bool success = false;
    int maxRetries = 3;
    
    for (int attempt = 1; attempt <= maxRetries; attempt++) {
      if (apiClient.sendHeartbeat()) {
        Serial.println("[Heartbeat] ✓ Heartbeat successful");
        success = true;
        break;
      } else {
        Serial.printf("[Heartbeat] ✗ Heartbeat failed (attempt %d/%d)\n", attempt, maxRetries);
        if (attempt < maxRetries) {
          delay(1000); // Esperar 1 segundo antes del siguiente intento
        }
      }
    }
    
    if (!success) {
      Serial.println("[Heartbeat] ✗ All heartbeat attempts failed");
    }
  }
}

void handleTimeSyncTask() {
  if (apiClient.isEnabled() && wifiManager.isConnected()) {
    if (timeManager.syncTime()) {
      // Enviar hora actualizada a los clientes web
      webServer.sendTimeInfo();
    }
  }
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
        case TASK_TURNSTILE_PERIODIC:
          handleTurnstilePeriodicTask();
          break;
        case TASK_API_HEARTBEAT:
          handleAPIHeartbeatTask();
          break;
        case TASK_TIME_SYNC:
          handleTimeSyncTask();
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
  
  // Cargar configuración (incluyendo modo)
  std::vector<WiFiNetwork> networks;
  String configMode;
  if (configManager.load(networks, configMode)) {
    wifiManager.setKnownNetworks(networks);
    
    // Si hay modo en config.json, usarlo
    if (configMode.length() > 0) {
      Serial.println("[Setup] Mode from config.json: " + configMode);
      modeManager.setMode(stringToMode(configMode));
    }
  }
  
  // Configurar API Client
  String apiBaseURL = configManager.getAPIBaseURL();
  bool apiEnabled = configManager.getAPIEnabled();
  unsigned long heartbeatInterval = configManager.getAPIHeartbeatInterval();
  
  if (!apiBaseURL.isEmpty()) {
    apiClient.setBaseURL(apiBaseURL);
    apiClient.setEnabled(apiEnabled);
    timeManager.setBaseURL(apiBaseURL);
    
    Serial.println("[Setup] API Configuration:");
    Serial.println("  Base URL: " + apiBaseURL);
    Serial.println("  Enabled: " + String(apiEnabled ? "Yes" : "No"));
    Serial.println("  Heartbeat Interval: " + String(heartbeatInterval / 1000) + "s");
    
    // Configurar intervalo de heartbeat
    tasks[TASK_API_HEARTBEAT].interval = heartbeatInterval;
    
    // NOTA: Las tareas de API se habilitarán automáticamente cuando se conecte WiFi
    // (ver callback wifiManager.setOnConnectedCallback)
  }
  
  // Inicializar Mode Manager (carga de /mode.txt si no hay en config.json)
  modeManager.begin();
  
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
  webServer.setModeManager(&modeManager);
  webServer.setTurnstileMode(&turnstileMode);
  webServer.setTimeManager(&timeManager);
  
  // Inicializar modo según configuración
  if (modeManager.getCurrentMode() == MODE_TURNSTILE) {
    Serial.println("Modo torniquete activo - inicializando SideConn");
    sideconn.begin();
    turnstileMode.begin();
    turnstileMode.setAutoLockDelay(TURNSTILE_AUTO_LOCK_DELAY);
    
    // Configurar callback para notificar eventos de acceso
    turnstileMode.setOnAccessEventCallback([](const String& uid, bool granted) {
      webServer.notifyAccessEvent(uid, granted);
    });
    
    tasks[TASK_TURNSTILE_PERIODIC].enabled = true;
  } else if (modeManager.getCurrentMode() == MODE_STANDALONE) {
    Serial.println("Modo standalone activo - solo identificación");
    standaloneMode.begin();
  }
  
  // Configurar callback para notificar conexiones WiFi exitosas
  wifiManager.setOnConnectedCallback([](const String& ssid, const String& ip) {
    Serial.println("Callback: WiFi conectado a " + ssid + " con IP " + ip);
    webServer.notifyWiFiConnected(ssid, ip);
    
    // Habilitar tareas de API cuando se conecta WiFi (si la API está habilitada)
    if (apiClient.isEnabled()) {
      Serial.println("[Setup] Enabling API tasks (heartbeat & time sync)");
      tasks[TASK_API_HEARTBEAT].enabled = true;
      tasks[TASK_TIME_SYNC].enabled = true;
      
      // Hacer un heartbeat inicial inmediatamente
      Serial.println("[Setup] Sending initial heartbeat...");
      if (apiClient.sendHeartbeat()) {
        Serial.println("[Setup] ✓ Initial heartbeat successful");
      } else {
        Serial.println("[Setup] ✗ Initial heartbeat failed");
      }
      
      // Sincronizar tiempo inmediatamente
      Serial.println("[Setup] Syncing time...");
      timeManager.syncTime();
    }
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
      
      // Habilitar tareas de API cuando se conecta WiFi (si la API está habilitada)
      if (apiClient.isEnabled()) {
        Serial.println("[Setup] Enabling API tasks (heartbeat & time sync)");
        tasks[TASK_API_HEARTBEAT].enabled = true;
        tasks[TASK_TIME_SYNC].enabled = true;
        
        // Hacer un heartbeat inicial inmediatamente
        Serial.println("[Setup] Sending initial heartbeat...");
        if (apiClient.sendHeartbeat()) {
          Serial.println("[Setup] ✓ Initial heartbeat successful");
        } else {
          Serial.println("[Setup] ✗ Initial heartbeat failed");
        }
        
        // Sincronizar tiempo inmediatamente
        Serial.println("[Setup] Syncing time...");
        timeManager.syncTime();
      }
    }
  }
  
  Serial.println("AP SSID: " + String(AP_SSID));
  if (wifiManager.isConnected()) {
    Serial.println("WiFi conectado a: " + wifiManager.getConnectedSSID());
    Serial.println("WiFi IP: " + wifiManager.getLocalIP().toString());
  }
  
  Serial.println("===========================================");
  Serial.println("Sistema inicializado");
  Serial.println("Modo actual: " + modeManager.getCurrentModeString());
  Serial.println("===========================================");
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

