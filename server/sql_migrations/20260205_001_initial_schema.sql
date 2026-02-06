-- Migration: 20260205_001_initial_schema
-- Description: Schema inicial para NeoAuth - Sensores, Usuarios y Tarjetas RFID
-- Date: 2026-02-05

-- Tabla de sensores RFID
CREATE TABLE IF NOT EXISTS sensors (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    sensor_id VARCHAR(100) UNIQUE NOT NULL,
    name VARCHAR(255) NOT NULL,
    location VARCHAR(255),
    mode VARCHAR(50) DEFAULT 'standalone' CHECK (mode IN ('standalone', 'turnstile')),
    is_active BOOLEAN DEFAULT true,
    last_heartbeat TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Tabla de usuarios
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE,
    is_active BOOLEAN DEFAULT true,
    metadata JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Tabla de tarjetas RFID
CREATE TABLE IF NOT EXISTS rfid_cards (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    uid VARCHAR(50) UNIQUE NOT NULL,
    user_id UUID REFERENCES users(id) ON DELETE CASCADE,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- Tabla de eventos de acceso
CREATE TABLE IF NOT EXISTS access_logs (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    sensor_id VARCHAR(100) NOT NULL,
    uid VARCHAR(50) NOT NULL,
    user_id UUID REFERENCES users(id) ON DELETE SET NULL,
    access_granted BOOLEAN NOT NULL,
    mode VARCHAR(50) NOT NULL,
    timestamp TIMESTAMPTZ DEFAULT NOW(),
    metadata JSONB DEFAULT '{}'
);

-- Índices para mejorar rendimiento
CREATE INDEX IF NOT EXISTS idx_sensors_sensor_id ON sensors(sensor_id);
CREATE INDEX IF NOT EXISTS idx_sensors_is_active ON sensors(is_active);
CREATE INDEX IF NOT EXISTS idx_rfid_cards_uid ON rfid_cards(uid);
CREATE INDEX IF NOT EXISTS idx_rfid_cards_user_id ON rfid_cards(user_id);
CREATE INDEX IF NOT EXISTS idx_access_logs_sensor_id ON access_logs(sensor_id);
CREATE INDEX IF NOT EXISTS idx_access_logs_timestamp ON access_logs(timestamp DESC);

-- Función para actualizar updated_at automáticamente
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Triggers para updated_at
CREATE TRIGGER update_sensors_updated_at BEFORE UPDATE ON sensors
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_users_updated_at BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

CREATE TRIGGER update_rfid_cards_updated_at BEFORE UPDATE ON rfid_cards
    FOR EACH ROW EXECUTE FUNCTION update_updated_at_column();

-- Datos de ejemplo (opcional, comentar en producción)
-- Sensor de ejemplo
INSERT INTO sensors (sensor_id, name, location, mode, is_active)
VALUES ('RFID_Reader_01', 'Sensor Principal', 'Entrada Principal', 'turnstile', true)
ON CONFLICT (sensor_id) DO NOTHING;

-- Usuario de ejemplo
INSERT INTO users (name, email, is_active)
VALUES ('Usuario Demo', 'demo@example.com', true)
ON CONFLICT (email) DO NOTHING;

-- Tarjeta de ejemplo (asociada al usuario demo)
INSERT INTO rfid_cards (uid, user_id, is_active)
VALUES ('AB:CD:EF:12', (SELECT id FROM users WHERE email = 'demo@example.com'), true)
ON CONFLICT (uid) DO NOTHING;
