-- Migration: 20260206_002_add_reports_table
-- Description: Tabla de reportes para estudiantes (retardos, salidas tempranas, etc)
-- Date: 2026-02-06

-- Tabla de reportes
CREATE TABLE IF NOT EXISTS reports (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    sensor_id VARCHAR(100) NOT NULL,
    report_type VARCHAR(50) NOT NULL CHECK (report_type IN ('late_entry', 'early_departure', 'manual_report', 'multiple_entry_attempt')),
    severity VARCHAR(20) DEFAULT 'warning' CHECK (severity IN ('info', 'warning', 'critical')),
    description TEXT NOT NULL,
    timestamp TIMESTAMPTZ DEFAULT NOW(),
    metadata JSONB DEFAULT '{}',
    resolved BOOLEAN DEFAULT false,
    resolved_at TIMESTAMPTZ,
    resolved_by UUID REFERENCES users(id) ON DELETE SET NULL,
    notes TEXT
);

-- Índices para búsquedas rápidas
CREATE INDEX idx_reports_user_id ON reports(user_id);
CREATE INDEX idx_reports_sensor_id ON reports(sensor_id);
CREATE INDEX idx_reports_type ON reports(report_type);
CREATE INDEX idx_reports_timestamp ON reports(timestamp DESC);
CREATE INDEX idx_reports_resolved ON reports(resolved);

-- Tabla de strikes acumulados (contador de faltas)
CREATE TABLE IF NOT EXISTS user_strikes (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    strike_type VARCHAR(50) NOT NULL CHECK (strike_type IN ('late_entry', 'early_departure')),
    count INTEGER DEFAULT 0,
    threshold INTEGER NOT NULL,
    last_strike_at TIMESTAMPTZ,
    reset_at TIMESTAMPTZ,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE(user_id, strike_type)
);

-- Índices para strikes
CREATE INDEX idx_strikes_user_id ON user_strikes(user_id);
CREATE INDEX idx_strikes_type ON user_strikes(strike_type);

COMMENT ON TABLE reports IS 'Registro de eventos reportables de estudiantes';
COMMENT ON TABLE user_strikes IS 'Contador acumulativo de faltas por usuario';
