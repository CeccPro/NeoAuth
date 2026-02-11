# Script para compilar el proyecto NeoAuth en Windows
# Uso: .\build.ps1 [-Device COM6]

param(
    [string]$Device = "COM6"
)

Write-Host "===========================================`n" -ForegroundColor Cyan
Write-Host "NeoAuth - Compilar Proyecto`n" -ForegroundColor Cyan
Write-Host "===========================================`n" -ForegroundColor Cyan

Write-Host "Compilando proyecto..." -ForegroundColor Yellow
platformio run

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[OK] Compilación exitosa`n" -ForegroundColor Green
} else {
    Write-Host "`n[ERROR] Error en la compilación`n" -ForegroundColor Red
    exit 1
}
