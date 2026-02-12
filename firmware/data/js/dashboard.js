// ===== DASHBOARD MODULE =====

window.updateDashboard = function() {
    updateMetricsCards();
    updateCharts();
};

function updateMetricsCards() {
    const metrics = AppState.systemMetrics;
    
    // RAM Usage
    const ramUsed = document.getElementById('ramUsed');
    const ramTotal = document.getElementById('ramTotal');
    const ramPercent = document.getElementById('ramPercent');
    
    if (ramUsed && ramTotal && ramPercent) {
        ramUsed.textContent = formatBytes(metrics.ram.used);
        ramTotal.textContent = formatBytes(metrics.ram.total);
        ramPercent.textContent = formatPercentage(metrics.ram.used, metrics.ram.total) + '%';
    }
    
    // Storage Usage
    const storageUsed = document.getElementById('storageUsed');
    const storageTotal = document.getElementById('storageTotal');
    const storagePercent = document.getElementById('storagePercent');
    
    if (storageUsed && storageTotal && storagePercent) {
        storageUsed.textContent = formatBytes(metrics.storage.used);
        storageTotal.textContent = formatBytes(metrics.storage.total);
        storagePercent.textContent = formatPercentage(metrics.storage.used, metrics.storage.total) + '%';
    }
    
    // CPU Usage
    const cpuUsage = document.getElementById('cpuUsage');
    if (cpuUsage) {
        cpuUsage.textContent = metrics.cpu + '%';
    }
    
    // WiFi Status
    const wifiStatusCard = document.getElementById('wifiStatusCard');
    const wifiStatusText = document.getElementById('wifiStatusText');
    
    if (wifiStatusCard && wifiStatusText && AppState.config) {
        if (AppState.config.wifi_connected) {
            wifiStatusCard.innerHTML = '<i class="bi bi-wifi"></i>';
            wifiStatusCard.className = 'stat-icon success';
            wifiStatusText.textContent = 'Conectado';
        } else {
            wifiStatusCard.innerHTML = '<i class="bi bi-wifi-off"></i>';
            wifiStatusCard.className = 'stat-icon danger';
            wifiStatusText.textContent = 'Desconectado';
        }
    }
}

function updateCharts() {
    const metrics = AppState.systemMetrics;
    
    // Actualizar RAM Chart
    if (window.ramChart && window.ramChart.data) {
        const ramUsed = metrics.ram.used || 0;
        const ramFree = (metrics.ram.total || 0) - ramUsed;
        window.ramChart.data.datasets[0].data = [ramUsed, ramFree];
        window.ramChart.update('none'); // 'none' para no animar
    }
    
    // Actualizar Storage Chart
    if (window.storageChart && window.storageChart.data) {
        const storageUsed = metrics.storage.used || 0;
        const storageFree = (metrics.storage.total || 0) - storageUsed;
        window.storageChart.data.datasets[0].data = [storageUsed, storageFree];
        window.storageChart.update('none');
    }
    
    // Actualizar CPU Chart
    if (window.cpuChart && window.cpuChart.data) {
        const cpuValue = metrics.cpu || 0;
        console.log('[Dashboard] Updating CPU chart with value:', cpuValue);
        window.cpuChart.data.datasets[0].data = [cpuValue];
        window.cpuChart.update('none');
    } else {
        console.warn('[Dashboard] CPU chart not initialized');
    }
}

window.updateClock = function() {
    const clockTime = document.getElementById('clockTime');
    const clockDate = document.getElementById('clockDate');
    
    if (!clockTime || !clockDate) return;
    
    // Usar hora local del navegador
    const now = new Date();
    const hours = now.getHours();
    const minutes = now.getMinutes().toString().padStart(2, '0');
    const seconds = now.getSeconds().toString().padStart(2, '0');
    const ampm = hours >= 12 ? 'PM' : 'AM';
    const displayHours = hours % 12 || 12;
    
    clockTime.textContent = `${displayHours}:${minutes}:${seconds} ${ampm}`;
    
    const days = ['domingo', 'lunes', 'martes', 'miércoles', 'jueves', 'viernes', 'sábado'];
    const months = ['enero', 'febrero', 'marzo', 'abril', 'mayo', 'junio', 'julio', 'agosto', 'septiembre', 'octubre', 'noviembre', 'diciembre'];
    clockDate.textContent = `${days[now.getDay()]}, ${now.getDate()} de ${months[now.getMonth()]} de ${now.getFullYear()}`;
};

function initCharts() {
    // Verificar que Chart.js esté disponible
    if (typeof Chart === 'undefined') {
        console.warn('[Dashboard] Chart.js not loaded yet, retrying...');
        setTimeout(initCharts, 100);
        return;
    }
    
    console.log('[Dashboard] Initializing charts...');
    
    // Destruir charts existentes si los hay
    if (window.ramChart && typeof window.ramChart.destroy === 'function') {
        window.ramChart.destroy();
    }
    if (window.storageChart && typeof window.storageChart.destroy === 'function') {
        window.storageChart.destroy();
    }
    if (window.cpuChart && typeof window.cpuChart.destroy === 'function') {
        window.cpuChart.destroy();
    }
    
    // RAM Chart
    const ramCtx = document.getElementById('ramChart');
    if (ramCtx) {
        window.ramChart = new Chart(ramCtx, {
            type: 'doughnut',
            data: {
                labels: ['Usado', 'Libre'],
                datasets: [{
                    data: [0, 100],
                    backgroundColor: ['#00d9ff', '#1a1a1a'],
                    borderColor: ['#00d9ff', '#333333'],
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: {
                            color: '#b0b0b0',
                            font: { size: 12 }
                        }
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                return context.label + ': ' + formatBytes(context.parsed);
                            }
                        }
                    }
                }
            }
        });
    }
    
    // Storage Chart
    const storageCtx = document.getElementById('storageChart');
    if (storageCtx) {
        window.storageChart = new Chart(storageCtx, {
            type: 'doughnut',
            data: {
                labels: ['Usado', 'Libre'],
                datasets: [{
                    data: [0, 100],
                    backgroundColor: ['#00ff88', '#1a1a1a'],
                    borderColor: ['#00ff88', '#333333'],
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: {
                            color: '#b0b0b0',
                            font: { size: 12 }
                        }
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                return context.label + ': ' + formatBytes(context.parsed);
                            }
                        }
                    }
                }
            }
        });
    }
    
    // CPU Gauge (usando bar chart horizontal)
    const cpuCtx = document.getElementById('cpuChart');
    if (cpuCtx) {
        window.cpuChart = new Chart(cpuCtx, {
            type: 'bar',
            data: {
                labels: ['CPU'],
                datasets: [{
                    label: 'Uso de CPU',
                    data: [0],
                    backgroundColor: '#ffaa00',
                    borderColor: '#ffaa00',
                    borderWidth: 2
                }]
            },
            options: {
                indexAxis: 'y',
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: {
                        beginAtZero: true,
                        max: 100,
                        ticks: { color: '#b0b0b0' },
                        grid: { color: '#333333' }
                    },
                    y: {
                        ticks: { color: '#b0b0b0' },
                        grid: { display: false }
                    }
                },
                plugins: {
                    legend: { display: false }
                }
            }
        });
    }
    
    console.log('[Dashboard] All charts initialized successfully');
}

// Exponer initCharts globalmente
window.initCharts = initCharts;
