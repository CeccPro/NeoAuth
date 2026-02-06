/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: metadata_parser.cpp
 * Author: CeccPro
 */

#include "metadata_parser.h"

MetadataParser::MetadataParser(const JsonObject& metadata)
  : metadata(metadata) {
}

// ============================================================================
// VERIFICACIONES BÁSICAS
// ============================================================================

bool MetadataParser::isAdmin() const {
  return metadata["is_admin"] | false;
}

bool MetadataParser::shouldShowInLogs() const {
  // Por defecto true, a menos que explícitamente se indique false
  return metadata["show_in_logs"] | true;
}

String MetadataParser::getRole() const {
  return metadata["role"] | "unknown";
}

String MetadataParser::getGrade() const {
  return metadata["grade"] | "N/A";
}

String MetadataParser::getGroup() const {
  return metadata["group"] | "N/A";
}

// ============================================================================
// VALIDACIÓN DE HORARIOS
// ============================================================================

ValidationResult MetadataParser::validateEntry(const String& currentTime, const String& currentDay) {
  ValidationResult result = {true, false, false, "OK", 0};
  
  // Admin siempre tiene acceso
  if (isAdmin()) {
    result.reason = "Admin - bypass all rules";
    return result;
  }
  
  // Si no hay reglas de acceso, permitir
  if (!hasAccessRules()) {
    result.reason = "No access rules defined";
    return result;
  }
  
  // Obtener rango de horario de entrada
  String timeRange = getTimeRange("entry_time", currentDay);
  if (timeRange.isEmpty()) {
    result.reason = "No entry schedule for " + currentDay;
    return result; // Permitir si no hay horario definido
  }
  
  // Parsear el rango
  String startTime, endTime;
  parseTimeRange(timeRange, startTime, endTime);
  
  int currentMinutes = timeToMinutes(currentTime);
  int startMinutes = timeToMinutes(startTime);
  int endMinutes = timeToMinutes(endTime);
  int maxLatenessMinutes = getMaxLatenessMinutes();
  int maxAllowedMinutes = endMinutes + maxLatenessMinutes;
  
  // Verificar si está dentro del horario normal
  if (currentMinutes >= startMinutes && currentMinutes <= endMinutes) {
    result.reason = "On time";
    return result;
  }
  
  // Verificar si está dentro del margen de tolerancia (late pero permitido)
  if (currentMinutes > endMinutes && currentMinutes <= maxAllowedMinutes) {
    result.isLate = true;
    result.minutesLate = currentMinutes - endMinutes;
    result.reason = "Late entry (within tolerance: " + String(result.minutesLate) + " min)";
    return result;
  }
  
  // Muy temprano o muy tarde
  result.allowed = false;
  if (currentMinutes < startMinutes) {
    result.reason = "Too early (before " + startTime + ")";
  } else {
    result.minutesLate = currentMinutes - endMinutes;
    result.reason = "Too late (exceeded max lateness by " + String(result.minutesLate - maxLatenessMinutes) + " min)";
  }
  
  return result;
}

ValidationResult MetadataParser::validateDeparture(const String& currentTime, const String& currentDay) {
  ValidationResult result = {true, false, false, "OK", 0};
  
  // Admin siempre tiene acceso
  if (isAdmin()) {
    result.reason = "Admin - bypass all rules";
    return result;
  }
  
  // Si no hay reglas de acceso, permitir
  if (!hasAccessRules()) {
    result.reason = "No access rules defined";
    return result;
  }
  
  // Obtener rango de horario de salida
  String timeRange = getTimeRange("departure_time", currentDay);
  if (timeRange.isEmpty()) {
    result.reason = "No departure schedule for " + currentDay;
    return result;
  }
  
  // Parsear el rango
  String startTime, endTime;
  parseTimeRange(timeRange, startTime, endTime);
  
  int currentMinutes = timeToMinutes(currentTime);
  int startMinutes = timeToMinutes(startTime);
  int endMinutes = timeToMinutes(endTime);
  
  // Verificar si está dentro del horario normal
  if (currentMinutes >= startMinutes && currentMinutes <= endMinutes) {
    result.reason = "On time";
    return result;
  }
  
  // Salida temprana (antes del horario permitido)
  if (currentMinutes < startMinutes) {
    result.isEarlyDeparture = true;
    result.minutesLate = startMinutes - currentMinutes;
    result.reason = "Early departure (" + String(result.minutesLate) + " min early)";
    return result;
  }
  
  // Salida tardía (después del horario) - siempre permitido sin penalización
  result.reason = "Late departure (allowed)";
  return result;
}

// ============================================================================
// CONFIGURACIONES
// ============================================================================

String MetadataParser::getEntryCooldown() const {
  if (!hasAccessRules()) return "";
  return metadata["access_rules"]["entry_cooldown"] | "";
}

int MetadataParser::getMaxLatenessMinutes() const {
  if (!hasAccessRules()) return 0;
  
  String maxLateness = metadata["access_rules"]["max_lateness"] | "0m";
  
  // Parsear "30m" -> 30
  int value = 0;
  if (maxLateness.endsWith("m")) {
    value = maxLateness.substring(0, maxLateness.length() - 1).toInt();
  }
  
  return value;
}

// ============================================================================
// STRIKES Y REPORTES
// ============================================================================

int MetadataParser::getStrikeThreshold(const String& type) const {
  if (metadata["strike_if"].isNull()) return 0;
  return metadata["strike_if"][type] | 0;
}

bool MetadataParser::shouldReport(const String& type) const {
  if (metadata["report_if"].isNull()) return false;
  return metadata["report_if"][type] | false;
}

// ============================================================================
// UTILIDADES
// ============================================================================

bool MetadataParser::hasAccessRules() const {
  return !metadata["access_rules"].isNull();
}

void MetadataParser::printDebugInfo() const {
  Serial.println("[MetadataParser] Debug Info:");
  Serial.println("  Role: " + getRole());
  Serial.println("  Is Admin: " + String(isAdmin() ? "Yes" : "No"));
  Serial.println("  Grade: " + getGrade());
  Serial.println("  Group: " + getGroup());
  Serial.println("  Has Access Rules: " + String(hasAccessRules() ? "Yes" : "No"));
  
  if (hasAccessRules()) {
    Serial.println("  Entry Cooldown: " + getEntryCooldown());
    Serial.println("  Max Lateness: " + String(getMaxLatenessMinutes()) + " min");
    Serial.println("  Late Entry Strike Threshold: " + String(getStrikeThreshold("late_entry")));
    Serial.println("  Early Departure Strike Threshold: " + String(getStrikeThreshold("early_departure")));
  }
}

// ============================================================================
// HELPERS INTERNOS
// ============================================================================

String MetadataParser::getTimeRange(const String& type, const String& day) const {
  if (!hasAccessRules()) return "";
  
  JsonObject timeConfig = metadata["access_rules"][type];
  if (timeConfig.isNull()) return "";
  
  // Buscar el día (ej: "monday")
  String dayLower = day;
  dayLower.toLowerCase();
  
  JsonArray ranges = timeConfig[dayLower];
  if (ranges.isNull() || ranges.size() == 0) return "";
  
  // Retornar el primer rango (ej: "06:30-07:00")
  return ranges[0] | "";
}

void MetadataParser::parseTimeRange(const String& range, String& start, String& end) const {
  int dashIndex = range.indexOf('-');
  if (dashIndex == -1) {
    start = range;
    end = range;
    return;
  }
  
  start = range.substring(0, dashIndex);
  end = range.substring(dashIndex + 1);
  
  // Limpiar espacios
  start.trim();
  end.trim();
}

int MetadataParser::timeToMinutes(const String& time) const {
  // Convertir "07:30" a minutos desde medianoche (450)
  int colonIndex = time.indexOf(':');
  if (colonIndex == -1) return 0;
  
  int hours = time.substring(0, colonIndex).toInt();
  int minutes = time.substring(colonIndex + 1).toInt();
  
  return hours * 60 + minutes;
}

String MetadataParser::minutesToTime(int minutes) const {
  int hours = minutes / 60;
  int mins = minutes % 60;
  
  char buffer[6];
  sprintf(buffer, "%02d:%02d", hours, mins);
  return String(buffer);
}
