/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: sideconn.cpp
 * Author: CeccPro
 */

#include "sideconn.h"

SideConn::SideConn(uint8_t sdaPin, uint8_t sclPin, uint8_t slaveAddress, MessageSize msgSize)
  : sdaPin(sdaPin), sclPin(sclPin), slaveAddress(slaveAddress),
    messageSize(msgSize), i2cFrequency(100000) {
}

void SideConn::begin() {
  Wire.begin(sdaPin, sclPin);
  Wire.setClock(i2cFrequency);
  
  Serial.println("[SideConn] I2C Initialized");
  Serial.println("  SDA: " + String(sdaPin));
  Serial.println("  SCL: " + String(sclPin));
  Serial.println("  Slave Address: 0x" + String(slaveAddress, HEX));
  Serial.println("  Frequency: " + String(i2cFrequency) + " Hz");
  
  // Verificar conectividad
  delay(100); // Dar tiempo al slave para inicializarse
  if (isConnected()) {
    Serial.println("[SideConn] Slave device detected");
  } else {
    Serial.println("[SideConn] WARNING: Slave device not responding");
  }
}

void SideConn::setClockSpeed(uint32_t frequency) {
  i2cFrequency = frequency;
  Wire.setClock(i2cFrequency);
  Serial.println("[SideConn] I2C frequency set to " + String(frequency) + " Hz");
}

void SideConn::setMessageSize(MessageSize size) {
  messageSize = size;
}

bool SideConn::isConnected() {
  Wire.beginTransmission(slaveAddress);
  uint8_t error = Wire.endTransmission();
  return (error == 0);
}

bool SideConn::sendMessage(const uint8_t* data) {
  if (!data) {
    Serial.println("[SideConn] ERROR: Null data pointer");
    return false;
  }
  
  Wire.beginTransmission(slaveAddress);
  Wire.write(data, messageSize);
  uint8_t error = Wire.endTransmission();
  
  if (error == 0) {
    return true;
  } else {
    Serial.println("[SideConn] ERROR: Failed to send message, I2C error code: " + String(error));
    return false;
  }
}

bool SideConn::sendMessage(const std::vector<uint8_t>& data) {
  if (data.size() != messageSize) {
    Serial.println("[SideConn] ERROR: Message size mismatch (expected " + 
                   String(messageSize) + ", got " + String(data.size()) + ")");
    return false;
  }
  
  return sendMessage(data.data());
}

bool SideConn::receiveMessage(uint8_t* buffer, unsigned long timeoutMs) {
  if (!buffer) {
    Serial.println("[SideConn] ERROR: Null buffer pointer");
    return false;
  }
  
  unsigned long startTime = millis();
  
  // Solicitar datos del slave
  uint8_t bytesReceived = Wire.requestFrom(slaveAddress, (uint8_t)messageSize);
  
  if (bytesReceived == 0) {
    Serial.println("[SideConn] ERROR: No data received from slave");
    return false;
  }
  
  // Leer datos disponibles
  size_t i = 0;
  while (Wire.available() && i < messageSize) {
    buffer[i++] = Wire.read();
    
    // Verificar timeout
    if (millis() - startTime > timeoutMs) {
      Serial.println("[SideConn] Receive timeout");
      return false;
    }
  }
  
  if (i != messageSize) {
    Serial.println("[SideConn] WARNING: Incomplete message received (" + 
                   String(i) + "/" + String(messageSize) + " bytes)");
    return false;
  }
  
  return true;
}

bool SideConn::receiveMessage(std::vector<uint8_t>& buffer, unsigned long timeoutMs) {
  buffer.resize(messageSize);
  return receiveMessage(buffer.data(), timeoutMs);
}

bool SideConn::sendAndReceive(const uint8_t* sendData, uint8_t* receiveBuffer, unsigned long timeoutMs) {
  if (!sendMessage(sendData)) {
    return false;
  }
  
  // Pequeña pausa para que el slave procese el comando
  delay(10);
  
  return receiveMessage(receiveBuffer, timeoutMs);
}
