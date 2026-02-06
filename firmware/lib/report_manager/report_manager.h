/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: report_manager.h
 * Author: CeccPro
 * 
 * Description:
 *   Report Manager - Handles student reports (late entries, early departures, etc)
 */

#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include <Arduino.h>
#include <api_client.h>

enum ReportType {
  REPORT_LATE_ENTRY,
  REPORT_EARLY_DEPARTURE,
  REPORT_MANUAL_REPORT,
  REPORT_MULTIPLE_ENTRY_ATTEMPT
};

enum ReportSeverity {
  SEVERITY_INFO,
  SEVERITY_WARNING,
  SEVERITY_CRITICAL
};

class ReportManager {
public:
  ReportManager(APIClient* apiClient, const String& sensorId);
  
  // Crear reporte
  bool submitReport(const String& uid, ReportType type, ReportSeverity severity, const String& details);
  
  // Helpers para tipos específicos
  bool reportLateEntry(const String& uid, int minutesLate);
  bool reportEarlyDeparture(const String& uid, int minutesEarly);
  bool reportManual(const String& uid, const String& description, ReportSeverity severity = SEVERITY_WARNING);
  
  // Estado
  bool isEnabled() const { return enabled; }
  void setEnabled(bool en) { enabled = en; }

private:
  APIClient* apiClient;
  String sensorId;
  bool enabled;
  
  String reportTypeToString(ReportType type) const;
  String severityToString(ReportSeverity severity) const;
};

#endif // REPORT_MANAGER_H
