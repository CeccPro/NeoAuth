/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: i2c_comm.h
 * Author: CeccPro
 * 
 * Description:
 *   I2C Communication wrapper for ESP32
 */

#ifndef I2C_COMM_H
#define I2C_COMM_H

#include <Arduino.h>
#include <Wire.h>
#include <vector>

class I2CComm {
public:
  I2CComm(uint8_t slaveAddress, uint8_t sdaPin = 21, uint8_t sclPin = 22);
  
  // Inicialización
  void begin();
  
  // Enviar comando al slave
  bool sendCommand(uint8_t cmd);
  bool sendMessage(const uint8_t* data, size_t len);
  
  // Recibir respuesta del slave
  bool receiveResponse(uint8_t* buffer, size_t len, unsigned long timeoutMs = 100);
  
  // Enviar y recibir
  bool sendAndReceive(uint8_t cmd, uint8_t* receiveBuffer, size_t receiveLen, unsigned long timeoutMs = 100);
  
  // Verificar conectividad
  bool isConnected();

private:
  uint8_t slaveAddress;
  uint8_t sdaPin;
  uint8_t sclPin;
};

#endif
