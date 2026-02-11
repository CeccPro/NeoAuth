# Script para monitorear el puerto serial en Windows
# Uso: .\monitor.ps1 [-Device COM6]

param(
    [string]$Device = "COM6"
)

Write-Host "===========================================`n" -ForegroundColor Cyan
Write-Host "NeoAuth - Monitor Serial`n" -ForegroundColor Cyan
Write-Host "===========================================`n" -ForegroundColor Cyan
Write-Host "Puerto: $Device" -ForegroundColor White
Write-Host "Presiona Ctrl+C para salir`n" -ForegroundColor White

platformio device monitor --port $Device
