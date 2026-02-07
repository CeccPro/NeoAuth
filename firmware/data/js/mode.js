// ===== MODE MODULE =====

let cardHistory = [];

window.updateModeInfo = function(data) {
    const currentMode = data.current_mode;
    
    // Mostrar/ocultar paneles según el modo
    const standalonePanel = document.getElementById('standalonePanel');
    const turnstilePanel = document.getElementById('turnstilePanel');
    
    if (standalonePanel && turnstilePanel) {
        if (currentMode === 'standalone') {
            standalonePanel.style.display = 'block';
            turnstilePanel.style.display = 'none';
        } else if (currentMode === 'turnstile') {
            standalonePanel.style.display = 'none';
            turnstilePanel.style.display = 'block';
        }
    }
};

window.loadModePanel = function() {
    renderCardHistory();
};

window.handleCardDetected = function(data) {
    const card = {
        uid: data.uid,
        timestamp: data.timestamp || new Date().toISOString(),
        time: new Date().toLocaleTimeString('es-MX'),
        user: data.user || null,
        access_granted: data.access_granted !== false
    };
    
    cardHistory.unshift(card);
    if (cardHistory.length > 50) cardHistory.pop();
    
    renderCardHistory();
    
    // Si estamos en modo standalone, mostrar info del usuario
    if (AppState.config.current_mode === 'standalone' && data.user) {
        updateStandaloneDisplay(data.user);
    }
};

function renderCardHistory() {
    const historyDiv = document.getElementById('cardHistory');
    
    if (!historyDiv) return;
    
    if (cardHistory.length === 0) {
        historyDiv.innerHTML = `
            <div class="empty-state">
                <i class="bi bi-inbox"></i>
                <p>Esperando detección de tarjetas...</p>
            </div>
        `;
        return;
    }
    
    historyDiv.innerHTML = '';
    cardHistory.forEach(card => {
        const item = document.createElement('div');
        item.className = 'log-item ' + (card.access_granted ? 'granted' : 'denied');
        
        let userInfo = '';
        if (card.user) {
            userInfo = `
                <div style="margin-top: 0.5rem; padding-top: 0.5rem; border-top: 1px solid var(--border);">
                    <strong>${card.user.name || 'Sin nombre'}</strong><br>
                    <small>UUID: ${card.user.id || '-'}</small><br>
                    ${card.user.metadata ? `
                        <small>
                            ${card.user.metadata.grado ? 'Grado: ' + card.user.metadata.grado : ''}
                            ${card.user.metadata.grupo ? ' | Grupo: ' + card.user.metadata.grupo : ''}
                            ${card.user.metadata.is_admin ? ' | <span class="badge bg-warning">Admin</span>' : ''}
                        </small>
                    ` : ''}
                </div>
            `;
        }
        
        item.innerHTML = `
            <div class="d-flex justify-content-between align-items-start">
                <div>
                    <div style="font-family: monospace; font-size: 1.1rem; font-weight: bold;">
                        <i class="bi bi-credit-card-2-front me-2"></i>${card.uid}
                    </div>
                    ${userInfo}
                </div>
                <div class="text-end">
                    <div style="font-size: 0.875rem; color: var(--text-secondary);">
                        <i class="bi bi-clock"></i> ${card.time}
                    </div>
                    <span class="badge ${card.access_granted ? 'bg-success' : 'bg-danger'} mt-1">
                        ${card.access_granted ? 'Acceso Permitido' : 'Acceso Denegado'}
                    </span>
                </div>
            </div>
        `;
        
        historyDiv.appendChild(item);
    });
}

function updateStandaloneDisplay(user) {
    const userDisplay = document.getElementById('standaloneUserDisplay');
    
    if (!userDisplay) return;
    
    userDisplay.innerHTML = `
        <div class="card">
            <div class="card-body text-center">
                <div class="mb-3">
                    <i class="bi bi-person-circle" style="font-size: 5rem; color: var(--accent);"></i>
                </div>
                <h3>${user.name || 'Usuario Desconocido'}</h3>
                <p class="text-muted">UUID: ${user.id || '-'}</p>
                ${user.metadata ? `
                    <div class="mt-3">
                        ${user.metadata.grado ? `<span class="badge bg-info me-2">Grado: ${user.metadata.grado}</span>` : ''}
                        ${user.metadata.grupo ? `<span class="badge bg-info me-2">Grupo: ${user.metadata.grupo}</span>` : ''}
                        ${user.metadata.is_admin ? `<span class="badge bg-warning">Administrador</span>` : ''}
                    </div>
                ` : ''}
            </div>
        </div>
    `;
}

function clearCardHistory() {
    if (confirm('¿Limpiar el historial de tarjetas?')) {
        cardHistory = [];
        renderCardHistory();
        showToast('Historial', 'Historial limpiado correctamente', 'success');
    }
}

// ===== TURNSTILE CONTROLS =====
function unlockTurnstile() {
    const duration = document.getElementById('unlockDuration').value;
    
    if (duration === 'indefinite') {
        if (confirm('¿Desbloquear el torniquete indefinidamente? El auto-lock se desactivará.')) {
            sendCommand('unlockTurnstile', { indefinite: true });
            showToast('Torniquete', 'Desbloqueando indefinidamente...', 'info');
        }
    } else {
        const seconds = parseInt(duration);
        if (confirm(`¿Desbloquear el torniquete por ${seconds} segundos?`)) {
            sendCommand('unlockTurnstile', { duration: seconds * 1000 });
            showToast('Torniquete', `Desbloqueando por ${seconds} segundos...`, 'info');
        }
    }
}

function lockTurnstile() {
    if (confirm('¿Bloquear el torniquete manualmente?')) {
        sendCommand('lockTurnstile', {});
        showToast('Torniquete', 'Bloqueando torniquete...', 'info');
    }
}

function toggleBlockMode() {
    const checkbox = document.getElementById('blockModeToggle');
    const enabled = checkbox.checked;
    
    if (enabled) {
        if (confirm('¿Activar modo de bloqueo? Todas las tarjetas serán rechazadas hasta desactivar este modo.')) {
            sendCommand('setBlockMode', { enabled: true });
            showToast('Torniquete', 'Modo de bloqueo ACTIVADO', 'warning');
        } else {
            checkbox.checked = false;
        }
    } else {
        sendCommand('setBlockMode', { enabled: false });
        showToast('Torniquete', 'Modo de bloqueo DESACTIVADO', 'success');
    }
}
