// ===== GLOBAL STATE =====
const AppState = {
    ws: null,
    config: {},
    connected: false,
    currentTab: 'dashboard',
    systemMetrics: {
        ram: { used: 0, total: 0 },
        storage: { used: 0, total: 0 },
        cpu: 0
    },
    rtc: {
        time: '00:00:00',
        date: '0000-00-00',
        synced: false
    }
};

// ===== WEBSOCKET CONNECTION =====
function connectWebSocket() {
    AppState.ws = new WebSocket('ws://' + window.location.hostname + '/ws');
    
    AppState.ws.onopen = function() {
        console.log('WebSocket conectado');
        updateSocketStatus(true);
        sendCommand('getConfig');
        sendCommand('getSystemMetrics');
    };
    
    AppState.ws.onclose = function() {
        console.log('WebSocket desconectado');
        updateSocketStatus(false);
        setTimeout(connectWebSocket, 3000);
    };
    
    AppState.ws.onerror = function(error) {
        console.error('Error WebSocket:', error);
        updateSocketStatus(false);
    };
    
    AppState.ws.onmessage = function(event) {
        try {
            const data = JSON.parse(event.data);
            handleMessage(data);
        } catch(e) {
            console.log('Mensaje recibido:', event.data);
        }
    };
}

function updateSocketStatus(connected) {
    AppState.connected = connected;
    const status = document.getElementById('socketStatus');
    if (!status) return;
    
    if (connected) {
        status.className = 'socket-status connected';
        status.innerHTML = '<i class="bi bi-circle-fill"></i> Conectado';
    } else {
        status.className = 'socket-status disconnected';
        status.innerHTML = '<i class="bi bi-circle-fill"></i> Desconectado';
    }
}

function sendCommand(command, params = {}) {
    if(AppState.ws && AppState.ws.readyState === WebSocket.OPEN) {
        const msg = {command: command, ...params};
        AppState.ws.send(JSON.stringify(msg));
    } else {
        showToast('Error', 'WebSocket no está conectado', 'error');
    }
}

function handleMessage(data) {
    // Distribuir mensajes a los módulos correspondientes
    if(data.type === 'system_info' || data.type === 'config') {
        AppState.config = data;
        if (window.updateSystemInfo) window.updateSystemInfo(data);
        if (window.updateNetworkInfo) window.updateNetworkInfo(data);
        if (window.updateModeInfo) window.updateModeInfo(data);
    }
    else if(data.type === 'system_metrics') {
        AppState.systemMetrics = {
            ram: data.ram || { used: 0, total: 0, free: 0 },
            storage: data.storage || { used: 0, total: 0, free: 0 },
            cpu: data.cpu || 0
        };
        if (window.updateDashboard) window.updateDashboard();
    }
    else if(data.type === 'rtc_time') {
        AppState.rtc = data;
        // El reloj se actualiza automáticamente cada segundo con JS
    }
    else if(data.type === 'access_event') {
        // Evento de acceso del torniquete
        const message = data.granted 
            ? `Acceso concedido: ${data.uid}` 
            : `Acceso denegado: ${data.uid}`;
        showToast('Torniquete', message, data.granted ? 'success' : 'error');
    }
    else if(data.type === 'identification_event') {
        // Evento de identificación (modo standalone)
        if (window.updateStandaloneDisplay) {
            if (data.found) {
                // Usuario encontrado - mostrar en panel superior
                const user = {
                    id: data.user_id || null,
                    name: data.user_name || 'Usuario Desconocido',
                    email: data.user_email || null,
                    metadata: data.user_metadata || {},
                    uid: data.uid
                };
                window.updateStandaloneDisplay(user, true);
                showToast('Identificación', `¡Bienvenido ${user.name}!`, 'success');
            } else {
                // Usuario NO encontrado - mostrar en panel superior
                const unknownUser = {
                    uid: data.uid,
                    name: null,
                    email: null,
                    metadata: null
                };
                window.updateStandaloneDisplay(unknownUser, false);
                showToast('Identificación', `Tarjeta no registrada: ${data.uid}`, 'warning');
            }
        }
    }
    else if(data.type === 'admin_card_event') {
        // Evento de detección de tarjeta en modo admin
        if (window.updateAdminCardDisplay) {
            const cardInfo = {
                uid: data.uid,
                user_name: data.user_name || '',
                user_email: data.user_email || '',
                role: data.role || 'user',
                is_active: data.is_active !== undefined ? data.is_active : true,
                metadata: data.metadata || {}
            };
            window.updateAdminCardDisplay(cardInfo, data.found);
        }
    }
    else if(data.type === 'success') {
        // Solo mostrar toast si no tiene requires_reboot
        if (!data.requires_reboot) {
            showToast('Éxito', data.message, 'success');
        }
        if(data.reboot) {
            showToast('Reiniciando', 'El dispositivo se reiniciará en 3 segundos...', 'warning');
        }
        sendCommand('getConfig');
    }
    else if(data.type === 'info') {
        showToast('Información', data.message, 'info');
    }
    else if(data.type === 'scan_started') {
        if (window.handleScanStarted) window.handleScanStarted();
    }
    else if(data.type === 'scan_result') {
        if (window.handleScanResult) window.handleScanResult(data.networks);
    }
    else if(data.type === 'wifi_connected') {
        showToast('WiFi', `Conectado a: ${data.ssid}`, 'success');
        sendCommand('getConfig');
    }
    else if(data.type === 'wifi_disconnected') {
        showToast('WiFi', 'Conexión WiFi perdida', 'warning');
        sendCommand('getConfig');
    }
    else if(data.type === 'error') {
        if(!data.message || !data.message.includes('escanear')) {
            showToast('Error', data.message, 'error');
        }
    }
}

// ===== TOAST SYSTEM =====
function showToast(title, message, type = 'info') {
    const toastId = 'toast-' + Date.now();
    const icons = {
        'success': 'bi-check-circle-fill',
        'error': 'bi-exclamation-circle-fill',
        'warning': 'bi-exclamation-triangle-fill',
        'info': 'bi-info-circle-fill'
    };
    
    const toast = document.createElement('div');
    toast.id = toastId;
    toast.className = `toast ${type}`;
    toast.setAttribute('role', 'alert');
    
    toast.innerHTML = `
        <div class="toast-header">
            <i class="bi ${icons[type]} me-2"></i>
            <strong class="me-auto">${title}</strong>
            <button type="button" class="btn-close" data-bs-dismiss="toast"></button>
        </div>
        <div class="toast-body">${message}</div>
    `;
    
    const container = document.getElementById('toastContainer');
    if (container) {
        container.appendChild(toast);
        const bsToast = new bootstrap.Toast(toast);
        bsToast.show();
        
        toast.addEventListener('hidden.bs.toast', () => {
            toast.remove();
        });
    }
}

// ===== TAB NAVIGATION =====
function switchTab(tabName) {
    AppState.currentTab = tabName;
    
    // Ocultar todos los tabs
    document.querySelectorAll('.tab-pane').forEach(pane => {
        pane.classList.remove('active', 'show');
    });
    
    // Mostrar el tab seleccionado
    const targetPane = document.getElementById(tabName + '-tab');
    if (targetPane) {
        targetPane.classList.add('active', 'show', 'fade-in');
    }
    
    // Actualizar nav links
    document.querySelectorAll('.nav-link').forEach(link => {
        link.classList.remove('active');
    });
    const activeLink = document.querySelector(`[data-tab="${tabName}"]`);
    if (activeLink) {
        activeLink.classList.add('active');
    }
    
    // Refrescar datos del tab activo
    if (tabName === 'dashboard') {
        // Inicializar gráficas si no están inicializadas
        if (window.initCharts && typeof window.ramChart === 'undefined') {
            window.initCharts();
        }
        if (window.updateDashboard) {
            window.updateDashboard();
        }
    } else if (tabName === 'networks' && window.loadNetworks) {
        window.loadNetworks();
    } else if (tabName === 'mode' && window.loadModePanel) {
        window.loadModePanel();
    }
}

// ===== UTILITY FUNCTIONS =====
function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
}

function formatPercentage(value, total) {
    if (total === 0) return 0;
    return Math.round((value / total) * 100);
}

// ===== INITIALIZATION =====
document.addEventListener('DOMContentLoaded', function() {
    console.log('NeoAuth Panel Inicializado');
    
    // Conectar WebSocket
    connectWebSocket();
    
    // Setup nav links
    document.querySelectorAll('.nav-link').forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            const tabName = this.getAttribute('data-tab');
            switchTab(tabName);
        });
    });
    
    // Inicializar charts PRIMERO (con retry si Chart.js no está listo)
    if (window.initCharts) {
        console.log('[App] Scheduling chart initialization');
        // Pequeño delay para asegurar que Chart.js esté cargado
        setTimeout(() => {
            window.initCharts();
        }, 100);
    }
    
    // Inicializar reloj inmediatamente
    if (window.updateClock) {
        window.updateClock();
        console.log('[App] Clock initialized');
    }
    
    // Cargar tab inicial DESPUÉS de inicializar charts
    switchTab('dashboard');
    
    // Actualizar métricas cada 5 segundos
    setInterval(() => {
        if (AppState.connected) {
            sendCommand('getSystemMetrics');
        }
    }, 5000);
    
    // Actualizar reloj cada segundo
    setInterval(() => {
        if (window.updateClock) {
            window.updateClock();
        }
    }, 1000);
});
