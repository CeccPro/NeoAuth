// ===== NETWORKS MODULE =====

window.updateNetworkInfo = function(data) {
    AppState.config = data;
    loadNetworks();
};

window.loadNetworks = function() {
    const networksList = document.getElementById('knownNetworksList');
    
    if (!networksList) return;
    
    const networks = AppState.config.known_networks || [];
    
    if (networks.length === 0) {
        networksList.innerHTML = `
            <div class="empty-state">
                <i class="bi bi-diagram-3"></i>
                <p>No hay redes configuradas</p>
            </div>
        `;
        return;
    }
    
    networksList.innerHTML = '';
    networks.forEach(net => {
        const item = document.createElement('div');
        item.className = 'network-item';
        
        const isConnected = AppState.config.wifi_connected && AppState.config.wifi_ssid === net.ssid;
        
        item.innerHTML = `
            <div class="network-info">
                <div class="network-ssid">
                    <i class="bi bi-wifi"></i>
                    ${net.ssid}
                    ${isConnected ? '<span class="badge bg-success ms-2">Conectada</span>' : ''}
                </div>
                <div class="network-meta">
                    ${net.has_password ? '<i class="bi bi-lock-fill"></i> Protegida' : '<i class="bi bi-unlock-fill"></i> Abierta'}
                </div>
            </div>
            <button class="btn btn-sm btn-danger" onclick="deleteNetwork('${net.ssid}')">
                <i class="bi bi-trash"></i>
            </button>
        `;
        
        networksList.appendChild(item);
    });
    
    // Actualizar estado WiFi en el card
    updateWiFiStatus();
};

function updateWiFiStatus() {
    const statusDiv = document.getElementById('wifiCurrentStatus');
    
    if (!statusDiv) return;
    
    if (AppState.config.wifi_connected) {
        statusDiv.innerHTML = `
            <div class="alert alert-success">
                <i class="bi bi-wifi me-2"></i>
                <strong>Conectado a:</strong> ${AppState.config.wifi_ssid}<br>
                <small>IP: ${AppState.config.wifi_ip}</small>
            </div>
        `;
    } else {
        statusDiv.innerHTML = `
            <div class="alert alert-warning">
                <i class="bi bi-wifi-off me-2"></i>
                <strong>Desconectado</strong><br>
                <small>No hay conexión WiFi activa</small>
            </div>
        `;
    }
}

function showAddNetworkModal() {
    const modal = new bootstrap.Modal(document.getElementById('addNetworkModal'));
    modal.show();
}

function addNetwork() {
    const ssid = document.getElementById('newSSID').value.trim();
    const password = document.getElementById('newPassword').value;
    
    if (!ssid) {
        showToast('Validación', 'Debes ingresar un SSID', 'warning');
        return;
    }
    
    if (ssid.length > 32) {
        showToast('Validación', 'El SSID no puede exceder 32 caracteres', 'warning');
        return;
    }
    
    if (password.length > 63) {
        showToast('Validación', 'La contraseña no puede exceder 63 caracteres', 'warning');
        return;
    }
    
    sendCommand('addWiFi', { ssid: ssid, password: password });
    
    document.getElementById('newSSID').value = '';
    document.getElementById('newPassword').value = '';
    bootstrap.Modal.getInstance(document.getElementById('addNetworkModal')).hide();
}

function deleteNetwork(ssid) {
    if (confirm(`¿Eliminar la red "${ssid}"?`)) {
        sendCommand('deleteWiFi', { ssid: ssid });
    }
}

function scanNetworks() {
    const modal = new bootstrap.Modal(document.getElementById('scanModal'));
    modal.show();
    
    document.getElementById('scanResults').innerHTML = `
        <div class="empty-state">
            <i class="bi bi-hourglass-split" style="animation: spin 1s linear infinite;"></i>
            <p>Escaneando redes WiFi...</p>
            <small class="text-muted">Esto puede tomar 10-20 segundos</small>
        </div>
    `;
    
    sendCommand('scanNetworks');
}

window.handleScanStarted = function() {
    document.getElementById('scanResults').innerHTML = `
        <div class="empty-state">
            <i class="bi bi-hourglass-split" style="animation: spin 1s linear infinite;"></i>
            <p>Escaneando redes WiFi...</p>
            <small class="text-muted">Esto puede tomar 10-20 segundos</small>
        </div>
    `;
};

window.handleScanResult = function(networks) {
    const list = document.getElementById('scanResults');
    
    if (networks.length === 0) {
        list.innerHTML = `
            <div class="empty-state">
                <i class="bi bi-wifi-off"></i>
                <p>No se encontraron redes WiFi</p>
            </div>
        `;
        return;
    }
    
    list.innerHTML = '';
    networks.forEach(net => {
        const item = document.createElement('div');
        item.className = 'network-item';
        
        let signalIcon = 'bi-wifi-1';
        if (net.rssi > -60) signalIcon = 'bi-wifi';
        else if (net.rssi > -70) signalIcon = 'bi-wifi-2';
        
        item.innerHTML = `
            <div class="network-info">
                <div class="network-ssid">
                    <i class="bi ${signalIcon}"></i>
                    ${net.ssid}
                </div>
                <div class="network-meta">
                    ${net.encryption !== 'Open' ? '<i class="bi bi-lock-fill"></i>' : '<i class="bi bi-unlock-fill"></i>'}
                    ${net.encryption}
                    <span style="margin-left: 0.5rem;">Señal: ${net.rssi} dBm</span>
                </div>
            </div>
            <button class="btn btn-sm btn-success" onclick="quickAddNetwork('${net.ssid}')">
                <i class="bi bi-plus-circle-fill"></i>
            </button>
        `;
        
        list.appendChild(item);
    });
};

function quickAddNetwork(ssid) {
    bootstrap.Modal.getInstance(document.getElementById('scanModal')).hide();
    document.getElementById('newSSID').value = ssid;
    showAddNetworkModal();
}

function connectToKnownNetwork() {
    sendCommand('connectWiFi');
    showToast('WiFi', 'Intentando conectar a red conocida...', 'info');
}
