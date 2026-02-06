#!/bin/bash

# NeoAuth - Installation Script
# Usage: ./install.sh [--server|--firmware|--all]

set -e

INSTALL_SERVER=false
INSTALL_FIRMWARE=false

# Procesar argumentos
if [ "$#" -eq 0 ]; then
    echo "Usage: ./install.sh [--server|--firmware|--all]"
    echo ""
    echo "Options:"
    echo "  --server     Install backend server dependencies"
    echo "  --firmware   Install firmware dependencies (PlatformIO)"
    echo "  --all        Install everything"
    exit 1
fi

for arg in "$@"; do
    case $arg in
        --server)
            INSTALL_SERVER=true
            ;;
        --firmware)
            INSTALL_FIRMWARE=true
            ;;
        --all)
            INSTALL_SERVER=true
            INSTALL_FIRMWARE=true
            ;;
        *)
            echo "Unknown option: $arg"
            exit 1
            ;;
    esac
done

# ============================================================================
# INSTALACIÓN DEL SERVIDOR
# ============================================================================

if [ "$INSTALL_SERVER" = true ]; then
    echo "========================================"
    echo "NeoAuth Server - Installation"
    echo "========================================"

    # Verificar si Node.js está instalado
    if ! command -v node &> /dev/null; then
        echo "❌ Node.js no está instalado"
        
        # Verificar si estamos en Railway o entorno de contenedor (sin sudo)
        if [ "$RAILWAY_ENVIRONMENT" != "" ] || ! command -v sudo &> /dev/null; then
            echo "⚠️  Entorno detectado sin privilegios de sudo (Railway/Container)"
            echo "Node.js debe ser instalado por el runtime del contenedor"
            echo "Verifica que el Dockerfile o buildpack incluya Node.js"
            exit 1
        fi
        
        echo "Instalando Node.js..."
        
        # Detectar distribución de Linux
        if [ -f /etc/debian_version ]; then
            # Debian/Ubuntu
            curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
            sudo apt-get install -y nodejs
        elif [ -f /etc/redhat-release ]; then
            # RedHat/CentOS/Fedora
            curl -fsSL https://rpm.nodesource.com/setup_20.x | sudo bash -
            sudo yum install -y nodejs
        else
            echo "⚠️  Distribución no reconocida. Instala Node.js manualmente:"
            echo "   https://nodejs.org/"
            exit 1
        fi
    fi

    # Verificar versión de Node.js
    NODE_VERSION=$(node -v | cut -d'v' -f2 | cut -d'.' -f1)
    if [ "$NODE_VERSION" -lt 18 ]; then
        echo "❌ Se requiere Node.js >= 18.x"
        echo "   Versión actual: $(node -v)"
        exit 1
    fi

    echo "✓ Node.js $(node -v) detectado"
    echo "✓ npm $(npm -v) detectado"

    # Cambiar a directorio del servidor
    cd server

    # Instalar dependencias
    echo ""
    echo "Instalando dependencias del servidor..."
    npm install

    # Crear archivo .env si no existe
    if [ ! -f .env ]; then
        echo ""
        echo "Creando archivo .env desde .env.example..."
        cp .env.example .env
        echo "⚠️  IMPORTANTE: Edita server/.env con tus credenciales de Supabase"
    fi

    cd ..

    echo ""
    echo "========================================"
    echo "✅ Servidor instalado correctamente"
    echo "========================================"
    echo ""
    echo "Próximos pasos:"
    echo "1. Configura tus credenciales en server/.env:"
    echo "   nano server/.env"
    echo ""
    echo "2. Ejecuta las migraciones SQL en Supabase:"
    echo "   Archivo: server/sql_migrations/20260205_001_initial_schema.sql"
    echo ""
    echo "3. Inicia el servidor:"
    echo "   ./startServer.sh"
    echo ""
fi

# ============================================================================
# INSTALACIÓN DEL FIRMWARE
# ============================================================================

if [ "$INSTALL_FIRMWARE" = true ]; then
    echo "========================================"
    echo "NeoAuth Firmware - Installation"
    echo "========================================"

    # Verificar si Python está instalado
    if ! command -v python3 &> /dev/null; then
        echo "❌ Python3 no está instalado"
        echo "Instalando Python3..."
        
        if [ -f /etc/debian_version ]; then
            sudo apt-get update
            sudo apt-get install -y python3 python3-pip python3-venv
        elif [ -f /etc/redhat-release ]; then
            sudo yum install -y python3 python3-pip
        fi
    fi

    echo "✓ Python3 $(python3 --version) detectado"

    # Instalar PlatformIO
    if ! command -v pio &> /dev/null; then
        echo ""
        echo "Instalando PlatformIO..."
        python3 -m pip install --user platformio
        
        # Agregar al PATH si no está
        if ! grep -q "/.platformio/penv/bin" ~/.bashrc; then
            echo 'export PATH=$PATH:~/.platformio/penv/bin' >> ~/.bashrc
            export PATH=$PATH:~/.platformio/penv/bin
        fi
    fi

    echo "✓ PlatformIO instalado"

    # Instalar dependencias del firmware
    echo ""
    echo "Configurando PlatformIO para el proyecto..."
    cd firmware
    pio pkg install
    cd ..

    echo ""
    echo "========================================"
    echo "✅ Firmware instalado correctamente"
    echo "========================================"
    echo ""
    echo "Próximos pasos:"
    echo "1. Conecta tu ESP32 via USB"
    echo "2. Compila y carga el firmware:"
    echo "   cd firmware && make upload"
    echo ""
fi

echo ""
echo "✅ Instalación completada"
