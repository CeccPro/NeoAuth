#!/bin/bash

# NeoAuth Server - Start Script

set -e

echo "========================================"
echo "NeoAuth Server - Starting..."
echo "========================================"

# Cambiar al directorio del servidor
cd server

# Verificar que .env existe
if [ ! -f .env ]; then
    echo "❌ Archivo .env no encontrado en server/"
    echo "   Copia server/.env.example a server/.env y configura tus credenciales"
    exit 1
fi

# Verificar que node_modules existe
if [ ! -d node_modules ]; then
    echo "⚠️  Dependencias no instaladas. Ejecutando npm install..."
    npm install
fi

# Iniciar servidor
echo "🚀 Iniciando servidor..."
npm start
