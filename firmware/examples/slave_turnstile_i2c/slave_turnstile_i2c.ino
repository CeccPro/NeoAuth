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
#include <Arduino.h>
#include <Servo.h>

#define I2C_SLAVE_ADDRESS 0x08
#define SERVO_PIN 9

// Ángulos del servomotor
#define SERVO_LOCKED 90      // Bloqueado
#define SERVO_UNLOCKED 0     // Desbloqueado

// Comandos (deben coincidir con el ESP32)
#define CMD_UNLOCK 0x01
#define CMD_LOCK   0x02
#define CMD_STATUS 0x03
#define CMD_PING   0x04

// Estado
bool locked = true;
unsigned long unlockTime = 0;       // Timestamp de cuándo se desbloqueó
unsigned long autoLockDelay = 0;    // Delay en milisegundos (0 = indefinido)
uint8_t lastCommand = 0x00;
uint8_t responseBuffer[4];
bool responseReady = false;

// Servomotor
Servo turnstileServo;

void setup() {
  Serial.begin(115200);
  
  // Inicializar servomotor
  turnstileServo.attach(SERVO_PIN);
  turnstileServo.write(SERVO_LOCKED);  // Posición inicial: bloqueado (90°)
  delay(500);  // Esperar a que el servo alcance la posición
  
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
}

void loop() {
  // Verificar auto-bloqueo (solo si autoLockDelay > 0)
  if (!locked && unlockTime > 0 && autoLockDelay > 0) {
    if (millis() - unlockTime >= autoLockDelay) {
      // Tiempo cumplido, bloquear automáticamente
      locked = true;
      turnstileServo.write(SERVO_LOCKED);
      unlockTime = 0;
      autoLockDelay = 0;
      Serial.println("AUTO-BLOQUEO: Torniquete bloqueado automáticamente");
    }
  }
  
  delay(10);
}

// Callback cuando el master envía datos
void receiveEvent(int numBytes) {
  if (numBytes >= 1) {
    uint8_t cmd = Wire.read();
    
    // Leer parámetro (2 bytes, big endian)
    uint16_t param = 0;
    if (numBytes >= 3) {
      uint8_t paramHigh = Wire.read();
      uint8_t paramLow = Wire.read();
      param = ((uint16_t)paramHigh << 8) | paramLow;
    }
    
    // Leer bytes restantes
    while (Wire.available()) {
      Wire.read();
    }
    
    Serial.print("Comando recibido: 0x");
    Serial.print(cmd, HEX);
    Serial.print(" con parámetro: 0x");
    Serial.println(param, HEX);
    
    processCommand(cmd, param);
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

void processCommand(uint8_t cmd, uint16_t param) {
  switch (cmd) {
    case CMD_UNLOCK:
      locked = false;
      unlockTime = millis();
      turnstileServo.write(SERVO_UNLOCKED);
      
      if (param == 0x0000) {
        // Desbloqueo indefinido
        autoLockDelay = 0;
        Serial.println("TORNIQUETE DESBLOQUEADO INDEFINIDAMENTE");
      } else {
        // Desbloqueo con tiempo específico (parámetro en segundos)
        autoLockDelay = (unsigned long)param * 1000UL;  // Convertir a milisegundos
        Serial.print("TORNIQUETE DESBLOQUEADO por ");
        Serial.print(param);
        Serial.println(" segundos");
      }
      break;
      
    case CMD_LOCK:
      locked = true;
      unlockTime = 0;
      autoLockDelay = 0;
      turnstileServo.write(SERVO_LOCKED);
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
