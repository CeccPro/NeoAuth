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
    if (window.ramChart) {
        const ramUsed = metrics.ram.used || 0;
        const ramFree = (metrics.ram.total || 0) - ramUsed;
        window.ramChart.data.datasets[0].data = [ramUsed, ramFree];
        window.ramChart.update('none'); // 'none' para no animar
    }
    
    // Actualizar Storage Chart
    if (window.storageChart) {
        const storageUsed = metrics.storage.used || 0;
        const storageFree = (metrics.storage.total || 0) - storageUsed;
        window.storageChart.data.datasets[0].data = [storageUsed, storageFree];
        window.storageChart.update('none');
    }
    
    // Actualizar CPU Chart
    if (window.cpuChart) {
        window.cpuChart.data.datasets[0].data = [metrics.cpu || 0];
        window.cpuChart.update('none');
    }
}

window.updateClock = function() {
    const clockTime = document.getElementById('clockTime');
    const clockDate = document.getElementById('clockDate');
    
    if (!clockTime || !clockDate) return;
    
    if (AppState.rtc.synced) {
        clockTime.textContent = AppState.rtc.time;
        clockDate.textContent = AppState.rtc.date;
    } else {
        // Fallback a hora local del navegador
        const now = new Date();
        clockTime.textContent = now.toLocaleTimeString('es-MX', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        clockDate.textContent = now.toLocaleDateString('es-MX', { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' });
    }
};

function initCharts() {
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
}

// Inicializar charts cuando el dashboard se carga
document.addEventListener('DOMContentLoaded', function() {
    initCharts();
});
