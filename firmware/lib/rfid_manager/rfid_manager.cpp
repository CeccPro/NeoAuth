/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: rfid_manager.cpp
 * Author: CeccPro
 * 
 * Description:
 *   RFID Manager Implementation
 */

#include "rfid_manager.h"

RFIDManager::RFIDManager(uint8_t ssPin, uint8_t rstPin)
  : rfid(ssPin, rstPin), ssPin(ssPin), rstPin(rstPin), lastCardReadTime(0) {
}

bool RFIDManager::begin() {
  SPI.begin(18, 19, 23, 5); // SCK, MISO, MOSI, SS
  rfid.PCD_Init();
  
  Serial.println("Running RFID connectivity tests");
  if (!testConnectivity()) {
    Serial.println("Sensor connectivity test failed");
    Serial.println("Check wiring, power supply and connections");
    return false;
  }
  
  Serial.println("RFID sensor initialized successfully");
  return true;
}

bool RFIDManager::testConnectivity() {
  // Test 1: Verificar versión del chip
  byte version = rfid.PCD_ReadRegister(rfid.VersionReg);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("Test 1: Chip version check failed");
    return false;
  }
  Serial.println("Test 1: Chip version: 0x" + String(version, HEX));
  
  // Test 2: Verificar comunicación SPI
  rfid.PCD_WriteRegister(rfid.TModeReg, 0x8D);
  byte testValue = rfid.PCD_ReadRegister(rfid.TModeReg);
  if (testValue != 0x8D) {
    Serial.println("Test 2: SPI communication check failed");
    return false;
  }
  Serial.println("Test 2: SPI communication check passed");
  
  // Reiniciar el sensor después del auto-test
  rfid.PCD_Init();
  
  return true;
}

bool RFIDManager::checkForCard(String& uid) {
  // Debounce para evitar lecturas múltiples
  unsigned long currentTime = millis();
  if (currentTime - lastCardReadTime < CARD_READ_DEBOUNCE) {
    return false;
  }
  
  // Verificar si hay una nueva tarjeta presente
  if (!rfid.PICC_IsNewCardPresent()) {
    return false;
  }
  
  // Seleccionar la tarjeta
  if (!rfid.PICC_ReadCardSerial()) {
    Serial.println("Failed to read card");
    return false;
  }
  
  // Construir el UID como string hexadecimal
  uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  
  Serial.println("Card detected - UID: " + uid);
  
  // Terminar la lectura de la tarjeta actual
  rfid.PICC_HaltA();
  
  // Actualizar tiempo de última lectura
  lastCardReadTime = currentTime;
  
  return true;
}

void RFIDManager::reset() {
  rfid.PCD_Init();
}
