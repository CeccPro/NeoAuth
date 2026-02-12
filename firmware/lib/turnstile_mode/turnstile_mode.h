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
#include <sideconn.h>
#include <api_client.h>
#include <vector>
#include <set>

// Comandos para el slave del torniquete
namespace TurnstileCmd {
  // Estructura de mensaje: 4 bytes
  // Byte 0: Comando
  // Byte 1-2: Parámetro (uint16_t, big endian)
  // Byte 3: Reservado
  
  const uint8_t CMD_UNLOCK = 0x01;  // Desbloquear torniquete (Byte 1-2: segundos, 0x0000 = indefinido)
  const uint8_t CMD_LOCK   = 0x02;  // Bloquear torniquete
  const uint8_t CMD_STATUS = 0x03;  // Solicitar estado
  const uint8_t CMD_PING   = 0x04;  // Ping de conectividad
  
  // Valor especial para desbloqueo indefinido
  const uint16_t UNLOCK_INDEFINITE = 0x0000;
}

class TurnstileMode {
public:
  TurnstileMode(SideConn* sideconn, APIClient* apiClient);
  
  // Inicialización
  void begin();
  
  // Verificar si una tarjeta está autorizada
  bool isCardAuthorized(const String& uid);
  
  // Procesar tarjeta detectada
  void handleCardDetected(const String& uid);
  
  // Gestión de tarjetas autorizadas (lista local temporal)
  void addAuthorizedCard(const String& uid);
  void removeAuthorizedCard(const String& uid);
  void clearAuthorizedCards();
  std::vector<String> getAuthorizedCards() const;
  
  // Control manual del torniquete
  bool unlockTurnstile();  // Usa autoLockDelay configurado
  bool unlockTurnstile(uint16_t seconds);  // Desbloquear por N segundos (0 = indefinido)
  bool lockTurnstile();
  bool pingSlave();
  
  // Configuración
  void setAutoLockDelay(unsigned long delayMs);
  unsigned long getAutoLockDelay() const { return autoLockDelay; }
  
  // Callback para notificar eventos
  typedef std::function<void(const String& uid, bool granted)> OnAccessEventCallback;
  void setOnAccessEventCallback(OnAccessEventCallback callback);
  
  // Tarea periódica (llamar en loop)
  void periodicTask();

private:
  SideConn* sideconn;
  APIClient* apiClient;
  std::set<String> authorizedCards;
  
  unsigned long autoLockDelay;  // Tiempo antes de auto-bloquear (ms)
  unsigned long unlockTime;     // Timestamp de último desbloqueo
  bool isUnlocked;
  
  OnAccessEventCallback onAccessEventCallback;
  
  // Enviar comando al slave
  bool sendCommand(uint8_t cmd);
  bool sendCommand(uint8_t cmd, uint16_t param);
  
  // Callbacks (se pueden extender para notificar eventos)
  void onAccessGranted(const String& uid);
  void onAccessDenied(const String& uid);
};

#endif // TURNSTILE_MODE_H
