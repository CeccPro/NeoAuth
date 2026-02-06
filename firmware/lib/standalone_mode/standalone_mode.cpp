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
  
  if (apiClient->whoIs(uid, found, userName, userEmail)) {
    if (found) {
      Serial.println("[StandaloneMode] ✓ User identified:");
      Serial.println("  Name: " + userName);
      Serial.println("  Email: " + userEmail);
    } else {
      Serial.println("[StandaloneMode] ✗ User not found in database");
    }
  } else {
    Serial.println("[StandaloneMode] ✗ API error - could not identify user");
  }
}
