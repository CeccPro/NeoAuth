/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: metadata_parser.h
 * Author: CeccPro
 * 
 * Description:
 *   Metadata Parser - Centralizes parsing and validation of user metadata
 */

#ifndef METADATA_PARSER_H
#define METADATA_PARSER_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct ValidationResult {
  bool allowed;
  bool isLate;
  bool isEarlyDeparture;
  String reason;
  int minutesLate;
};

class MetadataParser {
public:
  MetadataParser(const JsonObject& metadata);
  
  // Verificaciones básicas
  bool isAdmin() const;
  bool shouldShowInLogs() const;
  String getRole() const;
  String getGrade() const;
  String getGroup() const;
  
  // Validación de horarios
  ValidationResult validateEntry(const String& currentTime, const String& currentDay);
  ValidationResult validateDeparture(const String& currentTime, const String& currentDay);
  
  // Obtener configuraciones
  String getEntryCooldown() const;
  int getMaxLatenessMinutes() const;
  
  // Strikes y reportes
  int getStrikeThreshold(const String& type) const; // "late_entry" o "early_departure"
  bool shouldReport(const String& type) const;
  
  // Utilidades
  bool hasAccessRules() const;
  void printDebugInfo() const;

private:
  JsonObject metadata;
  
  // Helpers internos
  String getTimeRange(const String& type, const String& day) const; // type: "entry_time" o "departure_time"
  void parseTimeRange(const String& range, String& start, String& end) const;
  int calculateMinutesLate(const String& currentTime, const String& allowedTime) const;
  int timeToMinutes(const String& time) const;
  String minutesToTime(int minutes) const;
};

#endif // METADATA_PARSER_H
