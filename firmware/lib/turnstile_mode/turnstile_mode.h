/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: turnstile_mode.h
 * Author: CeccPro
 * 
 * Description:
 *   Turnstile Mode - Access control mode for physical turnstile
 */

#ifndef TURNSTILE_MODE_H
#define TURNSTILE_MODE_H

#include <Arduino.h>
#include <i2c_comm.h>
#include <api_client.h>
#include <time_manager.h>
#include <metadata_parser.h>
#include <report_manager.h>
#include <vector>
#include <set>

// Comandos para el slave del torniquete
namespace TurnstileCmd {
  // Estructura de mensaje: 4 bytes
  // Byte 0: Comando
  // Byte 1-3: Datos adicionales (por ahora en 0)
  
  const uint8_t CMD_UNLOCK = 0x01;  // Desbloquear torniquete
  const uint8_t CMD_LOCK   = 0x02;  // Bloquear torniquete
  const uint8_t CMD_STATUS = 0x03;  // Solicitar estado
  const uint8_t CMD_PING   = 0x04;  // Ping de conectividad
}

class TurnstileMode {
public:
  TurnstileMode(I2CComm* i2cComm, APIClient* apiClient, TimeManager* timeManager, ReportManager* reportManager);
  
  // Inicialización
  void begin();
  
  // Verificar si una tarjeta está autorizada (modo local)
  bool isCardAuthorized(const String& uid);
  
  // Procesar tarjeta detectada
  void handleCardDetected(const String& uid);
  
  // Gestión de tarjetas autorizadas (lista local temporal, fallback)
  void addAuthorizedCard(const String& uid);
  void removeAuthorizedCard(const String& uid);
  void clearAuthorizedCards();
  std::vector<String> getAuthorizedCards() const;
  
  // Control manual del torniquete
  bool unlockTurnstile();
  bool unlockIndefinitely();
  bool unlockForDuration(unsigned long durationMs);
  bool lockTurnstile();
  bool pingSlave();
  
  // Modo de bloqueo (rechazar todas las tarjetas)
  void setBlockMode(bool enabled);
  bool isBlockModeEnabled() const { return blockModeEnabled; }
  
  // Configuración
  void setAutoLockDelay(unsigned long delayMs);
  unsigned long getAutoLockDelay() const { return autoLockDelay; }
  
  // Tarea periódica (llamar en loop)
  void periodicTask();

private:
  I2CComm* i2cComm;
  APIClient* apiClient;
  TimeManager* timeManager;
  ReportManager* reportManager;
  std::set<String> authorizedCards;
  
  unsigned long autoLockDelay;  // Tiempo antes de auto-bloquear (ms)
  unsigned long unlockTime;     // Timestamp de último desbloqueo
  bool isUnlocked;
  bool autoLockEnabled;         // Si el auto-lock está activo
  bool blockModeEnabled;        // Modo de bloqueo total
  
  // Enviar comando al slave
  bool sendCommand(uint8_t cmd);
  
  // Callbacks (se pueden extender para notificar eventos)
  void onAccessGranted(const String& uid);
  void onAccessDenied(const String& uid);
};

#endif // TURNSTILE_MODE_H
