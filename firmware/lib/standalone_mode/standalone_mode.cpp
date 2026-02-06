/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: standalone_mode.cpp
 * Author: CeccPro
 */

#include "standalone_mode.h"

StandaloneMode::StandaloneMode(APIClient* apiClient)
  : apiClient(apiClient) {
}

void StandaloneMode::begin() {
  Serial.println("[StandaloneMode] Initializing standalone mode");
  Serial.println("[StandaloneMode] Mode: Identification only (no access control)");
  Serial.println("[StandaloneMode] Ready for card identification");
}

void StandaloneMode::handleCardDetected(const String& uid) {
  Serial.println("[StandaloneMode] Card detected: " + uid);
  
  if (!apiClient || !apiClient->isEnabled()) {
    Serial.println("[StandaloneMode] API not configured - card logged only");
    return;
  }
  
  bool found = false;
  String userName = "";
  String userEmail = "";
  JsonObject userMetadata;
  
  if (apiClient->whoIs(uid, found, userName, userEmail, userMetadata)) {
    if (found) {

      // Parsear metadatos
      /* Ejemplo de tarjeta de admin:
          {
            "role": "admin",
            "is_admin": true,
            "show_in_logs": false 
          }
          
          Nota: Si is_admin = true, tiene acceso total automáticamente.
          No necesita permissions ni access_rules explícitos.

          Ejemplo de tarjeta de estudiante:
          {
            "role": "student",
            "grade": "4",
            "group": "A",
            "access_rules": {
              "entry_cooldown": "once_per_day",
              "max_lateness": "30m",
              "entry_time": {
                "monday": ["06:30-7:00"],
                "tuesday": ["06:30-7:00"],
                "wednesday": ["06:30-7:00"],
                "thursday": ["06:30-7:00"],
                "friday": ["06:30-7:00"]
              },
              "departure_time": {
                "monday": ["12:00-15:00"],
                "tuesday": ["12:00-15:00"],
                "wednesday": ["12:00-15:00"],
                "thursday": ["12:00-15:00"],
                "friday": ["12:00-15:00"]
              }
            },
            "strike_if": {
              "late_entry": 3,
              "early_departure": 3
            },
            "report_if": {
              "late_entry": true,
              "early_departure": true
            }
          }
          
          Notas:
          - entry_time define los horarios permitidos para entrar. En este ejemplo, el estudiante puede entrar de lunes a viernes de 6:30 a 7:00 am.
          - max_lateness permite entrada retrasada dentro de ese margen (ej: horario 07:00, max_lateness 30m = puede entrar hasta 07:30)
          - Si entra después del max_lateness, se marca como "late_entry" en reportes (si report_if.late_entry = true)
          - Si sale antes de que termine su horario, se marca como "early_departure"
          - Salir tarde no tiene restricciones (el estudiante puede quedarse más tiempo sin problema)
          - entry_cooldown define reglas adicionales para controlar la frecuencia de entradas. En este ejemplo, "once_per_day" significa que el estudiante solo puede entrar una vez al día, incluso si su horario lo permite, y no podrá volver a entrar una vez que salga hasta el día siguiente
      */
      Serial.println("[StandaloneMode] ✓ User identified:");
      Serial.println("  Name: " + userName);
      Serial.println("  Email: " + userEmail);
      
      // Extraer información adicional si está disponible
      if (!userMetadata.isNull()) {
        String role = userMetadata["role"] | "N/A";
        String grade = userMetadata["grade"] | "N/A";
        String group = userMetadata["group"] | "N/A";
        bool isAdmin = userMetadata["is_admin"] | false;
        
        Serial.println("  Role: " + role);
        Serial.println("  Grade: " + grade);
        Serial.println("  Group: " + group);
        
        if (isAdmin) {
          Serial.println("  == Admin user ==");
        }
      }
    } else {
      Serial.println("[StandaloneMode] User not found in database");
    }
  } else {
    Serial.println("[StandaloneMode] API error - could not identify user");
  }
}
