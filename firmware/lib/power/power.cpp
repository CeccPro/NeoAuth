/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: power.cpp
 * Author: CeccPro
 * 
 * Description:
 *   This file contains power management functions for the device.
 *   It includes functions to safely power off the device and restart it.
 */

#include <Arduino.h>
#include "power.h"
#include <esp_system.h>
#include <esp_log.h>

static const char *TAG = "POWER";

void poweroff(int error_code) {
    // Aquí se pueden agregar procedimientos para guardar el estado, cerrar conexiones, etc.
    ESP_LOGI(TAG, "Apagando el dispositivo con código de error: %d", error_code);
    delay(100);

    // Deep sleep para indicar que el dispostivo ya puede apagarse
    esp_deep_sleep_start();
}

void restart(int error_code) {
    // Aquí se pueden agregar procedimientos para guardar el estado, cerrar conexiones, etc.
    ESP_LOGI(TAG, "Reiniciando el dispositivo con código de error: %d", error_code);
    
    // Reiniciar el dispositivo
    esp_restart();
}