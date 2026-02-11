# Script para cargar el firmware y monitorear en Windows
# Uso: .\deploy.ps1 [-Device COM6]

param(
    [string]$Device = "COM6"
)

Write-Host "===========================================`n" -ForegroundColor Cyan
Write-Host "NeoAuth - Deploy (Compilar + Cargar + Monitorear)`n" -ForegroundColor Cyan
Write-Host "===========================================`n" -ForegroundColor Cyan
Write-Host "Puerto: $Device`n" -ForegroundColor White

# Compilar
Write-Host "1. Compilando proyecto..." -ForegroundColor Yellow
platformio run

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[ERROR] Error en la compilación`n" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Compilación exitosa`n" -ForegroundColor Green

# Cargar filesystem
Write-Host "2. Cargando sistema de archivos..." -ForegroundColor Yellow
platformio run --target uploadfs --upload-port $Device

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[ERROR] Error cargando filesystem`n" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Filesystem cargado`n" -ForegroundColor Green

# Cargar firmware
Write-Host "3. Cargando firmware..." -ForegroundColor Yellow
platformio run --target upload --upload-port $Device

if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[ERROR] Error cargando firmware`n" -ForegroundColor Red
    exit 1
}
Write-Host "[OK] Firmware cargado`n" -ForegroundColor Green

# Monitorear
Write-Host "4. Abriendo monitor serial (Presiona Ctrl+C para salir)...`n" -ForegroundColor Yellow
platformio device monitor --port $Device
