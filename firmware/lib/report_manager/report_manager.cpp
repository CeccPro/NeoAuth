/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: report_manager.cpp
 * Author: CeccPro
 */

#include "report_manager.h"

ReportManager::ReportManager(APIClient* apiClient, const String& sensorId)
  : apiClient(apiClient), sensorId(sensorId), enabled(true) {
}

bool ReportManager::submitReport(const String& uid, ReportType type, ReportSeverity severity, const String& details) {
  if (!enabled || !apiClient || !apiClient->isEnabled()) {
    Serial.println("[ReportManager] Reports disabled or API not available");
    return false;
  }
  
  Serial.println("[ReportManager] Submitting report:");
  Serial.println("  UID: " + uid);
  Serial.println("  Type: " + reportTypeToString(type));
  Serial.println("  Severity: " + severityToString(severity));
  Serial.println("  Details: " + details);
  
  // Preparar payload
  DynamicJsonDocument payload(1024);
  payload["sensor_id"] = sensorId;
  payload["auth_token"] = apiClient->getAuthToken();
  payload["uid"] = uid;
  payload["report_type"] = reportTypeToString(type);
  payload["severity"] = severityToString(severity);
  payload["description"] = details;
  
  DynamicJsonDocument response(512);
  
  // Hacer request (reutilizamos makeRequest del APIClient)
  if (apiClient->makeRequest("/api/v1/reports", payload, response)) {
    String status = response["status"] | "";
    if (status == "ok") {
      Serial.println("[ReportManager] ✓ Report submitted successfully");
      return true;
    } else {
      Serial.println("[ReportManager] ✗ Report rejected: " + String(response["message"] | "Unknown error"));
      return false;
    }
  }
  
  Serial.println("[ReportManager] ✗ Failed to submit report");
  return false;
}

bool ReportManager::reportLateEntry(const String& uid, int minutesLate) {
  String details = "Student arrived " + String(minutesLate) + " minutes late";
  
  ReportSeverity severity = SEVERITY_INFO;
  if (minutesLate > 30) {
    severity = SEVERITY_WARNING;
  }
  if (minutesLate > 60) {
    severity = SEVERITY_CRITICAL;
  }
  
  return submitReport(uid, REPORT_LATE_ENTRY, severity, details);
}

bool ReportManager::reportEarlyDeparture(const String& uid, int minutesEarly) {
  String details = "Student left " + String(minutesEarly) + " minutes early";
  
  ReportSeverity severity = SEVERITY_INFO;
  if (minutesEarly > 30) {
    severity = SEVERITY_WARNING;
  }
  if (minutesEarly > 60) {
    severity = SEVERITY_CRITICAL;
  }
  
  return submitReport(uid, REPORT_EARLY_DEPARTURE, severity, details);
}

bool ReportManager::reportManual(const String& uid, const String& description, ReportSeverity severity) {
  return submitReport(uid, REPORT_MANUAL_REPORT, severity, description);
}

String ReportManager::reportTypeToString(ReportType type) const {
  switch (type) {
    case REPORT_LATE_ENTRY: return "late_entry";
    case REPORT_EARLY_DEPARTURE: return "early_departure";
    case REPORT_MANUAL_REPORT: return "manual_report";
    case REPORT_MULTIPLE_ENTRY_ATTEMPT: return "multiple_entry_attempt";
    default: return "unknown";
  }
}

String ReportManager::severityToString(ReportSeverity severity) const {
  switch (severity) {
    case SEVERITY_INFO: return "info";
    case SEVERITY_WARNING: return "warning";
    case SEVERITY_CRITICAL: return "critical";
    default: return "info";
  }
}
