/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: sideconn.h
 * Author: CeccPro
 * 
 * Description:
 *   SideConn - Simple bidirectional communication protocol
 *   3-wire protocol: CLK, DIN (data in from slave), DOUT (data out to slave)
 *   ESP32 acts as master, generates clock
 */

#ifndef SIDECONN_H
#define SIDECONN_H

#include <Arduino.h>
#include <vector>

// Tamaños de mensaje soportados
enum MessageSize {
  MSG_4_BYTES = 4,
  MSG_8_BYTES = 8
};

class SideConn {
public:
  SideConn(uint8_t clkPin, uint8_t dinPin, uint8_t doutPin, MessageSize msgSize = MSG_4_BYTES);
  
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
  void setClockSpeed(unsigned long delayUs); // Delay en microsegundos entre pulsos de reloj
  void setMessageSize(MessageSize size);
  MessageSize getMessageSize() const { return messageSize; }

private:
  uint8_t clkPin;
  uint8_t dinPin;
  uint8_t doutPin;
  MessageSize messageSize;
  unsigned long clockDelayUs;
  
  // Enviar/recibir un byte
  void sendByte(uint8_t byte);
  uint8_t receiveByte();
  
  // Generar pulso de reloj
  void clockPulse();
};

#endif // SIDECONN_H
