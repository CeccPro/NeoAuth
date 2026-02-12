/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: config.h
 * Author: CeccPro
 * 
 * Description:
 *   Global configuration and constants
 */

#ifndef CONFIG_H
#define CONFIG_H

// Información del sensor
#define RFID_SENSOR_ID "NEOAUTH_DEMO_SENSOR_001"
#define FIRMWARE_VERSION "0.1.2"

// Configuración del sensor RFID
#define SS_PIN  5
#define RST_PIN 22

// Configuración SPI
#define SPI_SCK  18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_SS   5

// Configuración del Access Point
#define AP_SSID "NEOAUTH_SENSOR_001"
#define AP_PASSWORD "12345678" // Contraseña del AP (mínimo 8 caracteres)

// Configuración del servidor web
#define WEB_SERVER_PORT 80

// Configuración serial
#define SERIAL_BAUD_RATE 115200
#define SERIAL_READ_TIMEOUT 100

// Secreto para generar tokens de autenticación
#define AUTH_SECRET "CeccProRules.IfYouKnowYouKnow.IfYouAreReadingThisYouAreGayLMAO"

// Configuración de archivos
#define CONFIG_FILE_PATH "/config.json"

// Configuración SideConn (comunicación I2C con slave)
#define SIDECONN_SDA_PIN      26
#define SIDECONN_SCL_PIN      25
#define SIDECONN_SLAVE_ADDR   0x08  // Dirección I2C del slave Arduino

// Configuración del modo torniquete
#define TURNSTILE_AUTO_LOCK_DELAY 5000  // 5 segundos antes de auto-bloquear

// Declaración de funciones globales
extern uint8_t getCPUUsage();

#endif // CONFIG_H
