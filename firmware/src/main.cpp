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
#include <i2c_comm.h>
#include <turnstile_mode.h>
#include <standalone_mode.h>
#include <api_client.h>
#include <time_manager.h>
#include <report_manager.h>
#include <sys/time.h>

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
WiFiManager wifiManager(AP_SSID, AP_PASSWORD);
ConfigManager configManager(CONFIG_FILE_PATH);
RFIDManager rfidManager(SS_PIN, RST_PIN);
WebServerManager webServer(WEB_SERVER_PORT);
ModeManager modeManager;
APIClient apiClient(RFID_SENSOR_ID, AUTH_SECRET);
TimeManager timeManager("");  // URL se configura después
ReportManager reportManager(&apiClient, RFID_SENSOR_ID);
I2CComm i2cComm(0x08, 26, 25);  // Dirección 0x08, SDA=Pin26, SCL=Pin25
TurnstileMode turnstileMode(&i2cComm, &apiClient, &timeManager, &reportManager);
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
  {TASK_TURNSTILE_PERIODIC,    0, 100,   false}, // Cada 100ms (solo en modo torniquete)
  {TASK_WEBSERVER_CLEANUP,     0, 2000,  true},  // Cada 2 segundos
  {TASK_API_HEARTBEAT,         0, 300000, false} // Cada 5 minutos (cuando API habilitada)
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
    // Notificar al web server
    webServer.notifyCardDetected(uid);
    
    // Manejar según el modo activo
    if (modeManager.getCurrentMode() == MODE_TURNSTILE) {
      turnstileMode.handleCardDetected(uid);
    } else if (modeManager.getCurrentMode() == MODE_STANDALONE) {
      standaloneMode.handleCardDetected(uid);
      webServer.notifyCardDetected(uid);
    }
  }
}

void handleAPIHeartbeat() {
  if (apiClient.isEnabled() && wifiManager.isConnected()) {
    time_t unixTime;
    if (apiClient.sendHeartbeat(unixTime) && unixTime > 0) {
      // Sincronizar RTC con la hora recibida
      struct timeval tv;
      tv.tv_sec = unixTime;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      
      Serial.println("[Main] RTC synchronized with server time");
      
      // Notificar al web server que hay nueva hora
      webServer.sendRTCTime();
    }
  }
}

void handleWebServerCleanup() {
  webServer.periodicTask();
}

void handleTurnstilePeriodicTask() {
  turnstileMode.periodicTask();
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
        case TASK_TURNSTILE_PERIODIC:
          handleTurnstilePeriodicTask();
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
        case TASK_API_HEARTBEAT:
          handleAPIHeartbeat();
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

  // Inicializar ConfigManager (SPIFFS) - PRIMERO para montar el filesystem
  if (!configManager.begin()) {
    Serial.println("ERROR: Failed to initialize SPIFFS");
    poweroff(2);
    for(;;);
  }
  
  // Ahora inicializar ModeManager (requiere SPIFFS montado y ConfigManager)
  Serial.println("Inicializando Mode Manager");
  modeManager = ModeManager(&configManager);
  modeManager.begin();
  
  // Cargar configuración
  std::vector<WiFiNetwork> networks;
  if (configManager.load(networks)) {
    wifiManager.setKnownNetworks(networks);
  }
  
  // Inicializar RFID Manager
  if (!rfidManager.begin()) {
    Serial.println("ERROR: RFID sensor initialization failed");
    poweroff(3);
    for(;;);
  }
  
  // Inicializar WiFi ANTES del WebServer
  Serial.println("Inicializando WiFi...");
  wifiManager.begin();
  
  // Inicializar el modo correspondiente
  if (modeManager.getCurrentMode() == MODE_TURNSTILE) {
    Serial.println("Modo torniquete activo - inicializando I2C");
    i2cComm.begin();
    turnstileMode.begin();
  } else if (modeManager.getCurrentMode() == MODE_STANDALONE) {
    Serial.println("Modo standalone activo - inicializando modo identificación");
    standaloneMode.begin();
  }
  
  // Configurar API Client
  String apiBaseURL = configManager.getAPIBaseURL();
  bool apiEnabled = configManager.getAPIEnabled();
  unsigned long heartbeatInterval = configManager.getAPIHeartbeatInterval();
  
  if (!apiBaseURL.isEmpty()) {
    apiClient.setBaseURL(apiBaseURL);
    apiClient.setEnabled(apiEnabled);
    tasks[TASK_API_HEARTBEAT].interval = heartbeatInterval;
    
    // Configurar TimeManager con la misma URL
    timeManager.setBaseURL(apiBaseURL);
    
    if (apiEnabled) {
      Serial.println("===========================================");
      Serial.println("API Client configurada");
      Serial.println("URL: " + apiBaseURL);
      Serial.println("Heartbeat: " + String(heartbeatInterval / 1000) + "s");
      Serial.println("===========================================");
      
      tasks[TASK_API_HEARTBEAT].enabled = true;
      
      if (wifiManager.isConnected()) {
        Serial.println("Probando conexión con API...");
        if (apiClient.testConnection()) {
          Serial.println("✓ API conectada correctamente");
          
          // Sincronizar tiempo desde el inicio
          Serial.println("Sincronizando hora con el servidor...");
          time_t unixTime;
          if (apiClient.sendHeartbeat(unixTime) && unixTime > 0) {
            struct timeval tv;
            tv.tv_sec = unixTime;
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
            Serial.println("✓ Hora sincronizada correctamente");
            webServer.sendRTCTime();
          } else {
            Serial.println("✗ No se pudo sincronizar la hora");
          }
        } else {
          Serial.println("✗ No se pudo conectar con la API");
        }
      }
    }
  }
  
  Serial.println("===========================================");
  Serial.println("Sistema inicializado");
  Serial.println("Modo actual: " + modeManager.getCurrentModeString());
  Serial.println("===========================================");
  
  // Configurar referencias antes de inicializar web server
  webServer.setModeManager(&modeManager);
  webServer.setTurnstileMode(&turnstileMode);
  
  // Inicializar Web Server (esto inicializa WiFi internamente)
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
  
  // Mostrar estado de WiFi
  Serial.println("AP SSID: " + String(AP_SSID));
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

