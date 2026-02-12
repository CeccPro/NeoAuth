// ===== CONFIG MODULE =====

window.updateSystemInfo = function(data) {
    // Actualizar información del sistema
    const sensorId = document.getElementById('configSensorId');
    const firmware = document.getElementById('configFirmware');
    
    if (sensorId) sensorId.textContent = data.sensor_id || '-';
    if (firmware) firmware.textContent = data.firmware_version || '-';
    
    // Actualizar configuración misc
    const modeSelect = document.getElementById('configMode');
    const heartbeatInput = document.getElementById('configHeartbeat');
    
    if (modeSelect && data.current_mode) {
        modeSelect.value = data.current_mode;
    }
    
    if (heartbeatInput && data.api && data.api.heartbeat_interval) {
        heartbeatInput.value = data.api.heartbeat_interval / 1000; // convertir a segundos
    }
    
    // Actualizar configuración de torniquete
    const autoLockInput = document.getElementById('configAutoLock');
    if (autoLockInput && data.turnstile) {
        autoLockInput.value = (data.turnstile.auto_lock_delay || 5000) / 1000;
    }
};

window.saveGeneralConfig = function() {
    const mode = document.getElementById('configMode').value;
    const heartbeat = document.getElementById('configHeartbeat').value;
    
    if (!mode || !heartbeat) {
        showToast('Validación', 'Completa todos los campos', 'warning');
        return;
    }
    
    const heartbeatMs = parseInt(heartbeat) * 1000;
    
    if (heartbeatMs < 60000) {
        showToast('Validación', 'El heartbeat debe ser al menos 60 segundos', 'warning');
        return;
    }
    
    // Verificar si el modo cambió
    const modeChanged = mode !== AppState.config.current_mode;
    
    if (modeChanged) {
        if (confirm('¿Cambiar el modo de operación? El dispositivo se reiniciará automáticamente.')) {
            // Primero cambiar el modo
            sendCommand('setMode', { mode: mode });
            
            // Mostrar mensaje de reinicio
            showToast('Sistema', 'Cambiando modo y reiniciando dispositivo...', 'info');
            
            // Reiniciar automáticamente después de 2 segundos
            setTimeout(() => {
                sendCommand('reboot');
            }, 2000);
        }
        return; // No continuar con la actualización de heartbeat si se cambia el modo
    }
    
    // Si solo se actualiza el heartbeat (sin cambio de modo)
    sendCommand('updateConfig', {
        api: {
            heartbeat_interval: heartbeatMs
        }
    });
    
    showToast('Configuración', 'Configuración guardada correctamente', 'success');
}

window.saveTurnstileConfig = function() {
    const autoLock = document.getElementById('configAutoLock').value;
    
    if (!autoLock) {
        showToast('Validación', 'Completa todos los campos', 'warning');
        return;
    }
    
    const autoLockMs = parseInt(autoLock) * 1000;
    
    if (autoLockMs < 1000) {
        showToast('Validación', 'El auto-lock debe ser al menos 1 segundo', 'warning');
        return;
    }
    
    sendCommand('updateConfig', {
        turnstile: {
            auto_lock_delay: autoLockMs
        }
    });
    
    showToast('Configuración', 'Configuración del torniquete guardada', 'success');
}

window.resetConfig = function() {
    if (confirm('¿Restaurar configuración de fábrica? Esta acción no se puede deshacer.')) {
        sendCommand('resetConfig');
        showToast('Configuración', 'Restableciendo configuración...', 'warning');
    }
}

window.rebootDevice = function() {
    if (confirm('¿Reiniciar el dispositivo? Esta acción tomará unos segundos.')) {
        sendCommand('reboot');
        showToast('Sistema', 'El dispositivo se reiniciará en 2 segundos...', 'warning');
        
        // Mostrar indicador de reconexión
        setTimeout(() => {
            showToast('Sistema', 'Esperando reconexión...', 'info');
        }, 3000);
    }
};
