/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: rfid_manager.h
 * Author: CeccPro
 * 
 * Description:
 *   RFID Manager - Handles RFID card reading and sensor testing
 */

#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

class RFIDManager {
public:
  RFIDManager(uint8_t ssPin, uint8_t rstPin);
  
  // Inicialización
  bool begin();
  
  // Test de conectividad del sensor
  bool testConnectivity();
  
  // Lectura de tarjetas (no bloqueante)
  bool checkForCard(String& uid);
  
  // Reiniciar el sensor
  void reset();

private:
  MFRC522 rfid;
  uint8_t ssPin;
  uint8_t rstPin;
  
  unsigned long lastCardReadTime;
  static const unsigned long CARD_READ_DEBOUNCE = 1000; // 1 segundo entre lecturas
};

#endif // RFID_MANAGER_H
