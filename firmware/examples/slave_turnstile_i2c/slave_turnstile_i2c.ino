/*
 * Slave Turnstile usando I2C
 * Hardware: Arduino Nano/Uno
 * 
 * Conexiones I2C:
 * - A4 (SDA) -> ESP32 Pin 26 (SDA)
 * - A5 (SCL) -> ESP32 Pin 25 (SCL)
 * - GND -> GND común
 * 
 * Dirección I2C del slave: 0x08
 */

#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x08
#define LED_PIN 2

// Comandos (deben coincidir con el ESP32)
#define CMD_UNLOCK 0x01
#define CMD_LOCK   0x02
#define CMD_STATUS 0x03
#define CMD_PING   0x04

// Configuración de auto-bloqueo
#define AUTO_LOCK_DELAY 5000  // 5 segundos en milisegundos

// Estado
bool locked = true;
unsigned long unlockTime = 0;  // Timestamp de cuándo se desbloqueó
uint8_t lastCommand = 0x00;
uint8_t responseBuffer[4];
bool responseReady = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  // Inicializar I2C como slave
  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  Serial.println("Slave I2C inicializado");
  Serial.print("Dirección: 0x");
  Serial.println(I2C_SLAVE_ADDRESS, HEX);
  
  // Estado inicial
  Serial.println("TORNIQUETE BLOQUEADO");
  locked = true;
  unlockTime = 0;
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  // Verificar auto-bloqueo
  if (!locked && unlockTime > 0) {
    if (millis() - unlockTime >= AUTO_LOCK_DELAY) {
      // Tiempo cumplido, bloquear automáticamente
      locked = true;
      digitalWrite(LED_PIN, HIGH);
      unlockTime = 0;
      Serial.println("AUTO-BLOQUEO: Torniquete bloqueado automáticamente");
    }
  }
  
  delay(10);
}

// Callback cuando el master envía datos
void receiveEvent(int numBytes) {
  if (numBytes > 0) {
    uint8_t cmd = Wire.read();
    
    // Leer bytes restantes (si los hay)
    while (Wire.available()) {
      Wire.read();
    }
    
    Serial.print("Comando recibido: 0x");
    Serial.println(cmd, HEX);
    
    processCommand(cmd);
  }
}

// Callback cuando el master solicita datos
void requestEvent() {
  if (responseReady) {
    Wire.write(responseBuffer, 4);
    Serial.println("Respuesta enviada");
    responseReady = false;
  } else {
    // Enviar respuesta por defecto
    uint8_t defaultResponse[4] = {0x00, 0x00, 0x00, 0x00};
    Wire.write(defaultResponse, 4);
  }
}

void processCommand(uint8_t cmd) {
  switch (cmd) {
    case CMD_UNLOCK:
      locked = false;
      unlockTime = millis();  // Guardar timestamp para auto-bloqueo
      digitalWrite(LED_PIN, LOW);
      Serial.println("TORNIQUETE DESBLOQUEADO (auto-bloqueo en 5s)");
      break;
      
    case CMD_LOCK:
      locked = true;
      unlockTime = 0;  // Cancelar auto-bloqueo
      digitalWrite(LED_PIN, HIGH);
      Serial.println("TORNIQUETE BLOQUEADO (manual)");
      break;
      
    case CMD_STATUS:
      Serial.println("Consulta de estado");
      prepareStatusResponse();
      break;
      
    case CMD_PING:
      Serial.println("PING recibido");
      preparePongResponse();
      break;
      
    default:
      Serial.println("Comando desconocido");
      break;
  }
}

void preparePongResponse() {
  responseBuffer[0] = 0xAA;                    // Confirmación
  responseBuffer[1] = 0x55;                    // Identificador
  responseBuffer[2] = locked ? 0x01 : 0x00;    // Estado
  responseBuffer[3] = 0x00;                    // Reservado
  responseReady = true;
}

void prepareStatusResponse() {
  responseBuffer[0] = locked ? 0x01 : 0x00;
  responseBuffer[1] = 0x00;
  responseBuffer[2] = 0x00;
  responseBuffer[3] = 0x00;
  responseReady = true;
}
