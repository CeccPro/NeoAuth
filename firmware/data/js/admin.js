// ===== ADMIN MODE MODULE =====

// Presets de metadata según rol
const ROLE_METADATA_PRESETS = {
    admin: {
        role: 'admin',
        permissions: ['read', 'write', 'delete', 'manage_users'],
        priority: 'high',
        show_in_logs: true
    },
    teacher: {
        role: 'teacher',
        permissions: ['read', 'write'],
        department: '',
        grade_level: '',
        show_in_logs: true
    },
    student: {
        role: 'student',
        permissions: ['read'],
        grade: '',
        group: '',
        student_id: '',
        show_in_logs: true
    },
    staff: {
        role: 'staff',
        permissions: ['read'],
        department: '',
        position: '',
        show_in_logs: true
    },
    guest: {
        role: 'guest',
        permissions: ['read'],
        valid_until: '',
        invited_by: '',
        show_in_logs: false
    }
};

function applyMetadataPreset(role) {
    const metadataTextarea = document.getElementById('adminCardMetadata');
    if (metadataTextarea && ROLE_METADATA_PRESETS[role]) {
        const preset = ROLE_METADATA_PRESETS[role];
        metadataTextarea.value = JSON.stringify(preset, null, 2);
    }
}

window.updateAdminInfo = function(data) {
    const currentMode = data.current_mode;
    const adminPanel = document.getElementById('adminPanel');
    
    if (adminPanel) {
        if (currentMode === 'admin') {
            adminPanel.style.display = 'block';
        } else {
            adminPanel.style.display = 'none';
        }
    }
};

window.updateAdminCardDisplay = function(cardInfo, found) {
    const display = document.getElementById('adminCardDisplay');
    if (!display) return;
    
    if (found) {
        // Tarjeta conocida - Mostrar formulario de edición
        display.innerHTML = `
            <div>
                <div class="alert alert-info">
                    <i class="bi bi-info-circle me-2"></i>
                    <strong>Tarjeta Registrada</strong> - Puedes editar la información
                </div>
                
                <div class="row">
                    <div class="col-md-6">
                        <div class="mb-3">
                            <label class="form-label">UID de la Tarjeta</label>
                            <input type="text" class="form-control" id="adminCardUID" value="${cardInfo.uid}" readonly>
                        </div>
                        
                        <div class="mb-3">
                            <label class="form-label">Nombre del Usuario</label>
                            <input type="text" class="form-control" id="adminUserName" value="${cardInfo.user_name || ''}" required>
                        </div>
                        
                        <div class="mb-3">
                            <label class="form-label">Email</label>
                            <input type="email" class="form-control" id="adminUserEmail" value="${cardInfo.user_email || ''}" placeholder="email@ejemplo.com">
                        </div>
                    </div>
                    
                    <div class="col-md-6">
                        <div class="mb-3">
                            <label class="form-label">Rol de la Tarjeta</label>
                            <select class="form-select" id="adminCardRole" onchange="applyMetadataPreset(this.value)">
                                <option value="admin" ${cardInfo.role === 'admin' ? 'selected' : ''}>Administrador</option>
                                <option value="teacher" ${cardInfo.role === 'teacher' ? 'selected' : ''}>Profesor</option>
                                <option value="student" ${cardInfo.role === 'student' ? 'selected' : ''}>Estudiante</option>
                                <option value="staff" ${cardInfo.role === 'staff' ? 'selected' : ''}>Personal</option>
                                <option value="guest" ${cardInfo.role === 'guest' ? 'selected' : ''}>Invitado</option>
                            </select>
                            <small class="text-muted">El rol define los permisos de esta tarjeta específica</small>
                        </div>
                        
                        <div class="mb-3">
                            <label class="form-label">Estado</label>
                            <select class="form-select" id="adminCardActive">
                                <option value="true" ${cardInfo.is_active ? 'selected' : ''}>Activa</option>
                                <option value="false" ${!cardInfo.is_active ? 'selected' : ''}>Desactivada</option>
                            </select>
                        </div>
                        
                        <div class="mb-3">
                            <label class="form-label">Metadatos (JSON)</label>
                            <textarea class="form-control" id="adminCardMetadata" rows="8" placeholder='{"role": "user"}'>${JSON.stringify(cardInfo.metadata || {}, null, 2)}</textarea>
                            <small class="text-muted">Información adicional en formato JSON. Cambia el rol para ver presets.</small>
                        </div>
                    </div>
                </div>
                
                <hr class="my-4">
                
                <div class="d-grid gap-2 d-md-flex justify-content-md-between">
                    <button class="btn btn-danger" onclick="deleteCard('${cardInfo.uid}')">
                        <i class="bi bi-trash-fill"></i> Desactivar Tarjeta
                    </button>
                    <div class="d-grid gap-2 d-md-flex">
                        <button class="btn btn-secondary" onclick="clearAdminForm()">
                            <i class="bi bi-x-circle"></i> Cancelar
                        </button>
                        <button class="btn btn-success" onclick="updateCard('${cardInfo.uid}')">
                            <i class="bi bi-check-circle"></i> Guardar Cambios
                        </button>
                    </div>
                </div>
            </div>
        `;
    } else {
        // Tarjeta nueva - Mostrar formulario de registro
        display.innerHTML = `
            <div>
                <div class="alert alert-warning">
                    <i class="bi bi-exclamation-triangle me-2"></i>
                    <strong>Tarjeta No Registrada</strong> - Completa el formulario para registrarla
                </div>
                
                <div class="row">
                    <div class="col-md-6">
                        <div class="mb-3">
                            <label class="form-label">UID de la Tarjeta</label>
                            <input type="text" class="form-control" id="adminCardUID" value="${cardInfo.uid || ''}" readonly>
                        </div>
                        
                        <div class="mb-3">
                            <label class="form-label">Nombre del Usuario *</label>
                            <input type="text" class="form-control" id="adminUserName" placeholder="Juan Pérez" required>
                        </div>
                        
                        <div class="mb-3">
                            <label class="form-label">Email</label>
                            <input type="email" class="form-control" id="adminUserEmail" placeholder="email@ejemplo.com">
                        </div>
                    </div>
                    
                    <div class="col-md-6">
                        <div class="mb-3">
                            <label class="form-label">Rol de la Tarjeta *</label>
                            <select class="form-select" id="adminCardRole" onchange="applyMetadataPreset(this.value)">
                                <option value="student" selected>Estudiante</option>
                                <option value="teacher">Profesor</option>
                                <option value="staff">Personal</option>
                                <option value="admin">Administrador</option>
                                <option value="guest">Invitado</option>
                            </select>
                            <small class="text-muted">El rol define los permisos de esta tarjeta</small>
                        </div>
                        
                        <div class="mb-3">
                            <label class="form-label">Metadatos (JSON)</label>
                            <textarea class="form-control" id="adminCardMetadata" rows="8" placeholder='{"role": "student"}'></textarea>
                            <small class="text-muted">Se auto-completa según el rol seleccionado. Puedes editar manualmente.</small>
                        </div>
                    </div>
                </div>
                
                <hr class="my-4">
                
                <div class="d-grid gap-2 d-md-flex justify-content-md-end">
                    <button class="btn btn-secondary" onclick="clearAdminForm()">
                        <i class="bi bi-x-circle"></i> Cancelar
                    </button>
                    <button class="btn btn-primary" onclick="registerCard('${cardInfo.uid}')">
                        <i class="bi bi-plus-circle"></i> Registrar Tarjeta
                    </button>
                </div>
            </div>
        `;
    }
};

window.registerCard = function(uid) {
    // Si no se proporciona UID como parámetro, intentar leerlo del input
    if (!uid || uid === '') {
        const uidInput = document.getElementById('adminCardUID');
        if (uidInput) {
            uid = uidInput.value.trim();
        }
    }
    
    const userName = document.getElementById('adminUserName').value.trim();
    const userEmail = document.getElementById('adminUserEmail').value.trim();
    const role = document.getElementById('adminCardRole').value;
    const metadataStr = document.getElementById('adminCardMetadata').value.trim();
    
    if (!uid) {
        showToast('Error', 'No se pudo obtener el UID de la tarjeta', 'error');
        return;
    }
    
    if (!userName) {
        showToast('Error', 'El nombre del usuario es obligatorio', 'error');
        return;
    }
    
    let metadata = {};
    if (metadataStr) {
        try {
            metadata = JSON.parse(metadataStr);
        } catch (e) {
            showToast('Error', 'Formato JSON inválido en metadatos', 'error');
            return;
        }
    }
    
    const data = {
        uid,
        user_name: userName,
        user_email: userEmail || null,
        role,
        metadata
    };
    
    sendCommand('registerCard', data);
    showToast('Procesando', 'Registrando tarjeta...', 'info');
};

window.updateCard = function(uid) {
    // Si no se proporciona UID como parámetro, intentar leerlo del input
    if (!uid || uid === '') {
        const uidInput = document.getElementById('adminCardUID');
        if (uidInput) {
            uid = uidInput.value.trim();
        }
    }
    
    const userName = document.getElementById('adminUserName').value.trim();
    const userEmail = document.getElementById('adminUserEmail').value.trim();
    const role = document.getElementById('adminCardRole').value;
    const isActive = document.getElementById('adminCardActive').value === 'true';
    const metadataStr = document.getElementById('adminCardMetadata').value.trim();
    
    if (!uid) {
        showToast('Error', 'No se pudo obtener el UID de la tarjeta', 'error');
        return;
    }
    
    if (!userName) {
        showToast('Error', 'El nombre del usuario es obligatorio', 'error');
        return;
    }
    
    let metadata = {};
    if (metadataStr) {
        try {
            metadata = JSON.parse(metadataStr);
        } catch (e) {
            showToast('Error', 'Formato JSON inválido en metadatos', 'error');
            return;
        }
    }
    
    const data = {
        uid,
        user_name: userName,
        user_email: userEmail || null,
        role,
        is_active: isActive,
        metadata
    };
    
    sendCommand('updateCard', data);
    showToast('Procesando', 'Actualizando tarjeta...', 'info');
};

window.deleteCard = function(uid) {
    // Si no se proporciona UID como parámetro, intentar leerlo del input
    if (!uid || uid === '') {
        const uidInput = document.getElementById('adminCardUID');
        if (uidInput) {
            uid = uidInput.value.trim();
        }
    }
    
    if (!uid) {
        showToast('Error', 'No se pudo obtener el UID de la tarjeta', 'error');
        return;
    }
    
    if (!confirm('¿Estás seguro de que deseas eliminar esta tarjeta?\n\nEsta acción no se puede deshacer.')) {
        return;
    }
    
    sendCommand('deleteCard', { uid });
    showToast('Procesando', 'Eliminando tarjeta...', 'info');
};

window.clearAdminForm = function() {
    const display = document.getElementById('adminCardDisplay');
    if (display) {
        display.innerHTML = `
            <div class="empty-state">
                <i class="bi bi-credit-card-2-front"></i>
                <p>Acerca una tarjeta para comenzar</p>
                <small class="text-muted">Las tarjetas conocidas se podrán editar, las nuevas se podrán registrar</small>
            </div>
        `;
    }
};
