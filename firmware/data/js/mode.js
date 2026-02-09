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
    
    // Crear imagen/avatar placeholder
    const avatarPlaceholder = user.metadata && user.metadata.avatar_url 
        ? `<img src="${user.metadata.avatar_url}" alt="Avatar" style="width: 120px; height: 120px; border-radius: 50%; object-fit: cover; border: 4px solid var(--accent);">`
        : `<div style="width: 120px; height: 120px; border-radius: 50%; background: linear-gradient(135deg, var(--accent-dark), var(--accent)); display: flex; align-items: center; justify-content: center; font-size: 3rem; color: var(--primary); font-weight: bold; border: 4px solid var(--accent); box-shadow: 0 8px 24px var(--glow);">${(user.name || 'U').charAt(0).toUpperCase()}</div>`;
    
    userDisplay.innerHTML = `
        <div style="text-align: center; padding: 2rem;">
            <div style="margin-bottom: 1.5rem; display: flex; justify-content: center;">
                ${avatarPlaceholder}
            </div>
            <h2 style="color: var(--accent); margin-bottom: 0.5rem; font-size: 2rem;">${user.name || 'Usuario Desconocido'}</h2>
            <p style="color: var(--text-secondary); font-family: monospace; margin-bottom: 1.5rem;">UUID: ${user.id || '-'}</p>
            ${user.email ? `<p style="color: var(--text-secondary); margin-bottom: 1rem;"><i class="bi bi-envelope"></i> ${user.email}</p>` : ''}
            ${user.metadata ? `
                <div style="display: flex; justify-content: center; gap: 0.5rem; flex-wrap: wrap; margin-top: 1rem;">
                    ${user.metadata.grado ? `<span class="badge bg-info" style="font-size: 0.9rem; padding: 0.5rem 1rem;">Grado: ${user.metadata.grado}</span>` : ''}
                    ${user.metadata.grupo ? `<span class="badge bg-info" style="font-size: 0.9rem; padding: 0.5rem 1rem;">Grupo: ${user.metadata.grupo}</span>` : ''}
                    ${user.metadata.is_admin ? `<span class="badge bg-warning" style="font-size: 0.9rem; padding: 0.5rem 1rem;"><i class="bi bi-shield-fill-check"></i> Administrador</span>` : ''}
                </div>
            ` : ''}
            <div style="margin-top: 2rem; padding: 1rem; background: var(--tertiary); border-radius: 8px; border-left: 4px solid var(--success);">
                <i class="bi bi-check-circle-fill" style="color: var(--success); font-size: 1.5rem;"></i>
                <p style="margin: 0.5rem 0 0; color: var(--success); font-weight: 600;">Usuario Identificado</p>
            </div>
        </div>
    `;
}

window.clearCardHistory = function() {
    if (confirm('¿Limpiar el historial de tarjetas?')) {
        cardHistory = [];
        renderCardHistory();
        showToast('Historial', 'Historial limpiado correctamente', 'success');
    }
};

// ===== TURNSTILE CONTROLS =====
window.unlockTurnstile = function() {
    const duration = document.getElementById('unlockDuration').value;
    
    if (duration === 'indefinite') {
        if (confirm('¿Desbloquear el torniquete indefinidamente? El auto-lock se desactivará.')) {
            sendCommand('unlockTurnstile', { duration: 9999 });
            showToast('Torniquete', 'Desbloqueando indefinidamente...', 'info');
        }
    } else {
        const seconds = parseInt(duration);
        if (confirm(`¿Desbloquear el torniquete por ${seconds} segundos?`)) {
            sendCommand('unlockTurnstile', { duration: seconds });
            showToast('Torniquete', `Desbloqueando por ${seconds} segundos...`, 'info');
        }
    }
}

window.lockTurnstile = function() {
    if (confirm('¿Bloquear el torniquete manualmente?')) {
        sendCommand('lockTurnstile');
        showToast('Torniquete', 'Bloqueando torniquete...', 'info');
    }
};

window.toggleBlockMode = function() {
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
