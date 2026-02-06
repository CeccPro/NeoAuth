/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: sideconn.cpp
 * Author: CeccPro
 */

#include "sideconn.h"

SideConn::SideConn(uint8_t clkPin, uint8_t dinPin, uint8_t doutPin, MessageSize msgSize)
  : clkPin(clkPin), dinPin(dinPin), doutPin(doutPin), 
    messageSize(msgSize), clockDelayUs(10) {
}

void SideConn::begin() {
  pinMode(clkPin, OUTPUT);
  pinMode(dinPin, INPUT_PULLUP);  // INPUT_PULLUP para evitar ruido
  pinMode(doutPin, OUTPUT);
  
  digitalWrite(clkPin, LOW);
  digitalWrite(doutPin, LOW);
  
  Serial.println("[SideConn] Initialized - CLK:" + String(clkPin) + 
                 " DIN:" + String(dinPin) + " DOUT:" + String(doutPin));
}

void SideConn::setClockSpeed(unsigned long delayUs) {
  clockDelayUs = delayUs;
}

void SideConn::setMessageSize(MessageSize size) {
  messageSize = size;
}

void SideConn::clockPulse() {
  delayMicroseconds(clockDelayUs);
  digitalWrite(clkPin, HIGH);
  delayMicroseconds(clockDelayUs);
  digitalWrite(clkPin, LOW);
}

void SideConn::sendByte(uint8_t byte) {
  // Enviar bit por bit, MSB primero
  for (int i = 7; i >= 0; i--) {
    // Escribir el bit en DOUT
    digitalWrite(doutPin, (byte >> i) & 0x01);
    
    // Generar pulso de reloj para que el slave lo lea
    clockPulse();
  }
  
  // Bajar DOUT al final para no dejarlo en HIGH
  digitalWrite(doutPin, LOW);
}

uint8_t SideConn::receiveByte() {
  uint8_t byte = 0;
  
  // Leer bit por bit, MSB primero
  for (int i = 7; i >= 0; i--) {
    // Subir CLK
    delayMicroseconds(clockDelayUs);
    digitalWrite(clkPin, HIGH);
    delayMicroseconds(clockDelayUs);
    
    // Leer el bit de DIN mientras CLK está HIGH
    if (digitalRead(dinPin)) {
      byte |= (1 << i);
    }
    
    // Bajar CLK
    digitalWrite(clkPin, LOW);
  }
  
  return byte;
}

bool SideConn::sendMessage(const uint8_t* data) {
  if (!data) return false;
  
  // Enviar byte de inicio para sincronización
  sendByte(0xAA);  // START byte
  
  // Enviar todos los bytes del mensaje
  for (int i = 0; i < messageSize; i++) {
    sendByte(data[i]);
  }
  
  return true;
}

bool SideConn::sendMessage(const std::vector<uint8_t>& data) {
  if (data.size() != messageSize) {
    Serial.println("[SideConn] ERROR: Message size mismatch");
    return false;
  }
  
  return sendMessage(data.data());
}

bool SideConn::receiveMessage(uint8_t* buffer, unsigned long timeoutMs) {
  if (!buffer) return false;
  
  unsigned long startTime = millis();
  
  // Esperar byte de inicio para sincronización
  uint8_t startByte = receiveByte();
  if (startByte != 0xAA) {
    Serial.println("[SideConn] ERROR: Invalid start byte: 0x" + String(startByte, HEX));
    return false;
  }
  
  // Recibir todos los bytes del mensaje
  for (int i = 0; i < messageSize; i++) {
    // Verificar timeout
    if (millis() - startTime > timeoutMs) {
      Serial.println("[SideConn] Receive timeout");
      return false;
    }
    
    buffer[i] = receiveByte();
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
  
  // Dar tiempo al slave para procesar y preparar respuesta (50ms)
  delay(50);
  
  return receiveMessage(receiveBuffer, timeoutMs);
}
