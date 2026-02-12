// ===== MODE MODULE =====

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
    // Ya no hay historial que renderizar
};

function updateStandaloneDisplay(user, found) {
    const userDisplay = document.getElementById('standaloneUserDisplay');
    
    if (!userDisplay) {
        return;
    }
    
    if (!found || !user.name) {
        // Usuario NO encontrado
        console.log('[updateStandaloneDisplay] Showing NOT FOUND for UID:', user.uid);
        userDisplay.innerHTML = `
            <div style="text-align: center; padding: 2rem;">
                <div style="margin-bottom: 1.5rem; display: flex; justify-content: center;">
                    <div style="width: 120px; height: 120px; border-radius: 50%; background: linear-gradient(135deg, var(--danger), #aa0033); display: flex; align-items: center; justify-content: center; font-size: 3rem; color: white; font-weight: bold; border: 4px solid var(--danger); box-shadow: 0 8px 24px rgba(255, 51, 102, 0.3);">
                        <i class="bi bi-person-x-fill"></i>
                    </div>
                </div>
                <h2 style="color: var(--danger); margin-bottom: 0.5rem; font-size: 2rem;">Usuario No Registrado</h2>
                <p style="color: var(--text-secondary); font-family: monospace; margin-bottom: 1.5rem; font-size: 1.1rem;">UID: ${user.uid || 'Desconocido'}</p>
                <div style="margin-top: 2rem; padding: 1rem; background: var(--tertiary); border-radius: 8px; border-left: 4px solid var(--danger);">
                    <i class="bi bi-x-circle-fill" style="color: var(--danger); font-size: 1.5rem;"></i>
                    <p style="margin: 0.5rem 0 0; color: var(--danger); font-weight: 600;">Tarjeta no encontrada en el sistema</p>
                </div>
            </div>
        `;
        return;
    }
    
    // Usuario encontrado
    console.log('[updateStandaloneDisplay] Showing FOUND user:', user.name);
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

window.updateStandaloneDisplay = updateStandaloneDisplay;

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
