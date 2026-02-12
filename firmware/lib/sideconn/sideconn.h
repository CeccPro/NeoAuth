/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: sideconn.h
 * Author: CeccPro
 * 
 * Description:
 *   SideConn - Physical connector interface for slave devices
 *   Uses I2C protocol for communication
 *   ESP32 acts as I2C master
 */

#ifndef SIDECONN_H
#define SIDECONN_H

#include <Arduino.h>
#include <Wire.h>
#include <vector>

// Tamaños de mensaje soportados
enum MessageSize {
  MSG_4_BYTES = 4,
  MSG_8_BYTES = 8
};

class SideConn {
public:
  // Constructor: sdaPin, sclPin, slaveAddress, messageSize
  SideConn(uint8_t sdaPin, uint8_t sclPin, uint8_t slaveAddress, MessageSize msgSize = MSG_4_BYTES);
  
  // Inicialización
  void begin();
  
  // Enviar mensaje al slave (blocking)
  bool sendMessage(const uint8_t* data);
  bool sendMessage(const std::vector<uint8_t>& data);
  
  // Recibir mensaje del slave (blocking con timeout)
  bool receiveMessage(uint8_t* buffer, unsigned long timeoutMs = 1000);
  bool receiveMessage(std::vector<uint8_t>& buffer, unsigned long timeoutMs = 1000);
  
  // Enviar y esperar respuesta
  bool sendAndReceive(const uint8_t* sendData, uint8_t* receiveBuffer, unsigned long timeoutMs = 1000);
  
  // Configuración
  void setClockSpeed(uint32_t frequency); // Frecuencia I2C en Hz (100000, 400000, etc)
  void setMessageSize(MessageSize size);
  MessageSize getMessageSize() const { return messageSize; }
  
  // Verificar conectividad
  bool isConnected();

private:
  uint8_t sdaPin;
  uint8_t sclPin;
  uint8_t slaveAddress;
  MessageSize messageSize;
  uint32_t i2cFrequency;
};

#endif // SIDECONN_H
