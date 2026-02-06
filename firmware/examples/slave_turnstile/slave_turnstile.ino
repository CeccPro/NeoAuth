/*
 * Ejemplo usando SideConnSlave Library
 * Hardware: Arduino Nano/Uno o similar
 * 
 * Conexiones:
 * - Pin 2 (CLK)  -> ESP32 Pin 25 (requiere soporte de interrupciones)
 * - Pin 3 (DIN)  -> ESP32 Pin 27 (DOUT del ESP32)
 * - Pin 4 (DOUT) -> ESP32 Pin 26 (DIN del ESP32)
 * - GND -> GND común
 */

#include "SideConnSlave.h"

#define CLK_PIN    2
#define DIN_PIN    3
#define DOUT_PIN   4
#define LED_PIN    13

// Comandos (deben coincidir con el ESP32)
#define CMD_UNLOCK 0x01
#define CMD_LOCK   0x02
#define CMD_STATUS 0x03
#define CMD_PING   0x04

// Crear instancia de SideConn
SideConnSlave sideconn(CLK_PIN, DIN_PIN, DOUT_PIN);

// Estado
bool locked = true;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  
  // Inicializar SideConn
  sideconn.begin();
  
  Serial.println("Slave inicializado - esperando comandos...");
  
  // Estado inicial
  locked = true;
  digitalWrite(LED_PIN, locked ? HIGH : LOW);
  
  // Configurar interrupción
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), onClock, FALLING);
}

void loop() {
  // Procesar SideConn (delays y flags internos)
  sideconn.process();
  
  // Verificar si hay mensaje
  if (sideconn.available()) {
    uint8_t message[4];
    if (sideconn.readMessage(message)) {
      processCommand(message[0], &message[1]);
    }
  }
}

// Interrupt handler
void onClock() {
  sideconn.handleClockInterrupt();
}

void processCommand(uint8_t cmd, uint8_t* data) {
  Serial.print("Comando recibido: 0x");
  Serial.println(cmd, HEX);
  
  switch (cmd) {
    case CMD_UNLOCK:
      locked = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("TORNIQUETE DESBLOQUEADO");
      break;
      
    case CMD_LOCK:
      locked = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("TORNIQUETE BLOQUEADO");
      break;
      
    case CMD_STATUS:
      sendStatus();
      break;
      
    case CMD_PING:
      Serial.println("PING recibido - esperando sincronización...");
      delay(30);  // Esperar a que el master esté listo para leer
      sendPong();
      Serial.println("PONG enviado");
      break;
      
    default:
      Serial.println("Comando desconocido");
      break;
  }
}

void sendPong() {
  uint8_t response[4] = {
    0xAA,                    // Confirmación
    0x55,                    // Identificador
    locked ? 0x01 : 0x00,    // Estado
    0x00                     // Reservado
  };
  
  sideconn.prepareResponse(response);
}

void sendStatus() {
  Serial.print("Estado: ");
  Serial.println(locked ? "BLOQUEADO" : "DESBLOQUEADO");
}
