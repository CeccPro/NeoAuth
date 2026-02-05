/*
 * Copyright (c) 2026 CeccPro. All rights reserved.
 * Filename: power.h
 * Author: CeccPro
 * 
 * Description:
 *   This file contains power management functions for the device.
 *   It includes functions to safely power off the device and restart it.
 */

#ifndef POWER_H
#define POWER_H

/**
 * @brief Prepara el dispositivo para apagarse de manera segura
 * @param error_code Código de error para el apagado (0 si es normal)
 */
void poweroff(int error_code);

/**
 * @brief Reinicia el dispositivo
 * @param error_code Código de error para el reinicio (0 si es normal)
 */
void restart(int error_code);

#endif // POWER_H