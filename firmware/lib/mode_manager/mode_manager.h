/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: mode_manager.h
 * Author: CeccPro
 * 
 * Description:
 *   Mode Manager - Handles different operational modes for the sensor
 */

#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <Arduino.h>

// Modos disponibles del sensor
enum SensorMode {
  MODE_STANDALONE,   // Modo standalone: solo lectura y log
  MODE_TURNSTILE,    // Modo torniquete: control de acceso físico
  MODE_REGISTRATION, // Modo registro: registrar nuevas tarjetas
  MODE_SYNC          // Modo sincronización: sync con backend
};

// Convertir modo a string
const char* modeToString(SensorMode mode);

// Convertir string a modo
SensorMode stringToMode(const String& modeStr);

class ModeManager {
public:
  ModeManager();
  
  // Inicialización
  void begin();
  
  // Cambiar modo
  bool setMode(SensorMode newMode);
  
  // Obtener modo actual
  SensorMode getCurrentMode() const { return currentMode; }
  String getCurrentModeString() const { return String(modeToString(currentMode)); }
  
  // Guardar/cargar modo desde configuración
  bool saveMode();
  bool loadMode();

private:
  SensorMode currentMode;
  const char* CONFIG_MODE_FILE = "/mode.txt";
};

#endif // MODE_MANAGER_H
