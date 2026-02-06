/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: i2c_comm.cpp
 * Author: CeccPro
 */

#include "i2c_comm.h"

I2CComm::I2CComm(uint8_t slaveAddress, uint8_t sdaPin, uint8_t sclPin)
  : slaveAddress(slaveAddress), sdaPin(sdaPin), sclPin(sclPin) {
}

void I2CComm::begin() {
  Wire.begin(sdaPin, sclPin);
  Wire.setClock(100000); // 100kHz standard mode
  
  Serial.println("[I2CComm] Initialized - SDA:" + String(sdaPin) + 
                 " SCL:" + String(sclPin) + 
                 " Slave:0x" + String(slaveAddress, HEX));
}

bool I2CComm::sendCommand(uint8_t cmd) {
  Wire.beginTransmission(slaveAddress);
  Wire.write(cmd);
  uint8_t error = Wire.endTransmission();
  
  if (error == 0) {
    return true;
  } else {
    Serial.println("[I2CComm] ERROR: Failed to send command, error code: " + String(error));
    return false;
  }
}

bool I2CComm::sendMessage(const uint8_t* data, size_t len) {
  if (!data || len == 0) return false;
  
  Wire.beginTransmission(slaveAddress);
  Wire.write(data, len);
  uint8_t error = Wire.endTransmission();
  
  return (error == 0);
}

bool I2CComm::receiveResponse(uint8_t* buffer, size_t len, unsigned long timeoutMs) {
  if (!buffer || len == 0) return false;
  
  unsigned long startTime = millis();
  
  // Solicitar datos del slave
  uint8_t bytesReceived = Wire.requestFrom(slaveAddress, len);
  
  if (bytesReceived == 0) {
    Serial.println("[I2CComm] ERROR: No data received from slave");
    return false;
  }
  
  // Leer datos disponibles
  size_t i = 0;
  while (Wire.available() && i < len) {
    buffer[i++] = Wire.read();
    
    // Verificar timeout
    if (millis() - startTime > timeoutMs) {
      Serial.println("[I2CComm] Receive timeout");
      return false;
    }
  }
  
  return (i == len);
}

bool I2CComm::sendAndReceive(uint8_t cmd, uint8_t* receiveBuffer, size_t receiveLen, unsigned long timeoutMs) {
  // Enviar comando
  if (!sendCommand(cmd)) {
    return false;
  }
  
  // Pequeño delay para que el slave procese el comando
  delay(10);
  
  // Recibir respuesta
  return receiveResponse(receiveBuffer, receiveLen, timeoutMs);
}

bool I2CComm::isConnected() {
  Wire.beginTransmission(slaveAddress);
  uint8_t error = Wire.endTransmission();
  return (error == 0);
}
