import express from 'express';
import cors from 'cors';
import helmet from 'helmet';
import rateLimit from 'express-rate-limit';
import dotenv from 'dotenv';
import { createClient } from '@supabase/supabase-js';
import crypto from 'crypto';

dotenv.config();

const app = express();
const PORT = process.env.PORT || 3000;

// Supabase client
const supabase = createClient(
  process.env.SUPABASE_URL,
  process.env.SUPABASE_SERVICE_ROLE_KEY
);

// Middleware
app.use(helmet());
app.use(cors({ origin: process.env.CORS_ORIGIN || '*' }));
app.use(express.json());

// Rate limiting
const limiter = rateLimit({
  windowMs: 1 * 60 * 1000, // 1 minuto
  max: 100, // 100 requests por ventana
  message: 'Too many requests from this IP'
});
app.use('/api/', limiter);

// ============================================================================
// UTILIDADES
// ============================================================================

/**
 * Genera token de autenticación basado en AUTH_SECRET + SensorID
 */
function generateAuthToken(sensorId) {
  const secret = process.env.AUTH_SECRET;
  return crypto.createHmac('sha256', secret)
    .update(sensorId)
    .digest('hex');
}

/**
 * Valida el token de autenticación
 */
function validateAuthToken(sensorId, providedToken) {
  const expectedToken = generateAuthToken(sensorId);
  return crypto.timingSafeEqual(
    Buffer.from(expectedToken),
    Buffer.from(providedToken)
  );
}

/**
 * Middleware de autenticación
 */
async function authenticateSensor(req, res, next) {
  const { sensor_id, auth_token } = req.body;

  if (!sensor_id || !auth_token) {
    return res.status(401).json({
      status: 'error',
      message: 'Missing sensor_id or auth_token'
    });
  }

  // Validar token
  try {
    if (!validateAuthToken(sensor_id, auth_token)) {
      return res.status(403).json({
        status: 'error',
        message: 'Invalid authentication token'
      });
    }
  } catch (error) {
    return res.status(403).json({
      status: 'error',
      message: 'Invalid authentication token'
    });
  }

  // Verificar que el sensor existe y está activo
  const { data: sensor, error } = await supabase
    .from('sensors')
    .select('*')
    .eq('sensor_id', sensor_id)
    .single();

  if (error || !sensor) {
    return res.status(404).json({
      status: 'error',
      message: 'Sensor not found'
    });
  }

  if (!sensor.is_active) {
    return res.status(403).json({
      status: 'error',
      message: 'Sensor has been revoked'
    });
  }

  // Agregar sensor al request para uso posterior
  req.sensor = sensor;
  next();
}

// ============================================================================
// ENDPOINTS
// ============================================================================

/**
 * Health check
 */
app.get('/health', (req, res) => {
  res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

/**
 * GET /api/v1/time
 * Devuelve la hora actual del servidor con timezone
 */
app.get('/api/v1/time', (req, res) => {
  const now = new Date();
  
  // Ajustar a timezone de México (UTC-6)
  const mexicoTime = new Date(now.toLocaleString('en-US', { timeZone: 'America/Mexico_City' }));
  
  res.json({
    status: 'ok',
    timestamp: now.toISOString(),
    time: mexicoTime.toTimeString().split(' ')[0], // "HH:MM:SS"
    date: mexicoTime.toISOString().split('T')[0],   // "YYYY-MM-DD"
    day: mexicoTime.toLocaleDateString('en-US', { weekday: 'lowercase' }), // "monday"
    timezone: 'America/Mexico_City',
    unix: Math.floor(now.getTime() / 1000)
  });
});

/**
 * POST /api/v1/heartbeat
 * Recibe heartbeat de sensores y devuelve hora actual
 */
app.post('/api/v1/heartbeat', authenticateSensor, async (req, res) => {
  const { sensor_id } = req.body;

  // Actualizar last_heartbeat
  const { error } = await supabase
    .from('sensors')
    .update({ last_heartbeat: new Date().toISOString() })
    .eq('sensor_id', sensor_id);

  if (error) {
    console.error('Error updating heartbeat:', error);
    return res.status(500).json({
      status: 'error',
      message: 'Failed to update heartbeat'
    });
  }

  // Enviar hora actual en respuesta
  const now = new Date();
  const mexicoTime = new Date(now.toLocaleString('en-US', { timeZone: 'America/Mexico_City' }));
  
  res.json({
    status: 'ok',
    message: 'Heartbeat received',
    timestamp: now.toISOString(),
    time: mexicoTime.toTimeString().split(' ')[0], // "HH:MM:SS"
    date: mexicoTime.toISOString().split('T')[0],   // "YYYY-MM-DD"
    day: mexicoTime.toLocaleDateString('en-US', { weekday: 'lowercase' }), // "monday"
    timezone: 'America/Mexico_City',
    unix: Math.floor(now.getTime() / 1000)
  });
});

/**
 * POST /api/v1/access
 * Valida acceso de tarjeta RFID (modo turnstile)
 */
app.post('/api/v1/access', authenticateSensor, async (req, res) => {
  const { sensor_id, uid, timestamp } = req.body;

  if (!uid) {
    return res.status(400).json({
      status: 'error',
      message: 'Missing uid parameter'
    });
  }

  // Buscar tarjeta y usuario
  const { data: card, error: cardError } = await supabase
    .from('rfid_cards')
    .select(`
      *,
      users (
        id,
        name,
        email,
        is_active
      )
    `)
    .eq('uid', uid)
    .eq('is_active', true)
    .single();

  // Logging para debug
  console.log('Card query result:', { card, cardError });
  
  // Determinar razón de denegación
  let denialReason = '';
  let accessGranted = false;
  
  if (cardError) {
    denialReason = 'Card not found or inactive';
  } else if (!card) {
    denialReason = 'Card not found';
  } else if (!card.users) {
    denialReason = 'User not found for this card';
  } else if (!card.users.is_active) {
    denialReason = 'User is inactive';
  } else {
    accessGranted = true;
  }

  // Registrar evento
  await supabase.from('access_logs').insert({
    sensor_id,
    uid,
    user_id: card?.user_id || null,
    access_granted: accessGranted,
    mode: 'turnstile',
    timestamp: timestamp || new Date().toISOString(),
    metadata: { denial_reason: denialReason || 'Access granted' }
  });

  if (!accessGranted) {
    console.log('Access denied:', denialReason);
    return res.json({
      status: 'denied',
      message: denialReason,
      uid
    });
  }

  res.json({
    status: 'ok',
    message: 'Access granted',
    uid,
    user: {
      id: card.users.id,
      name: card.users.name,
      metadata: card.users.metadata || {}
    }
  });
});

/**
 * POST /api/v1/who_is
 * Identifica usuario por UID de tarjeta (modo standalone)
 */
app.post('/api/v1/who_is', authenticateSensor, async (req, res) => {
  const { sensor_id, uid, timestamp } = req.body;

  if (!uid) {
    return res.status(400).json({
      status: 'error',
      message: 'Missing uid parameter'
    });
  }

  // Buscar tarjeta y usuario
  const { data: card, error } = await supabase
    .from('rfid_cards')
    .select(`
      *,
      users (
        id,
        name,
        email,
        is_active,
        metadata
      )
    `)
    .eq('uid', uid)
    .single();

  // Registrar evento
  await supabase.from('access_logs').insert({
    sensor_id,
    uid,
    user_id: card?.user_id || null,
    access_granted: !error && card?.is_active && card?.users?.is_active,
    mode: 'standalone',
    timestamp: timestamp || new Date().toISOString()
  });

  if (error || !card) {
    return res.json({
      status: 'not_found',
      message: 'Card not found',
      uid
    });
  }

  if (!card.is_active || !card.users?.is_active) {
    return res.json({
      status: 'inactive',
      message: 'Card or user is inactive',
      uid
    });
  }

  res.json({
    status: 'ok',
    uid,
    user: {
      id: card.users.id,
      name: card.users.name,
      email: card.users.email,
      metadata: card.users.metadata
    }
  });
});

/**
 * POST /api/v1/reports
 * Crear reporte para un usuario
 */
app.post('/api/v1/reports', authenticateSensor, async (req, res) => {
  const { sensor_id, uid, report_type, severity, description, metadata } = req.body;

  if (!uid || !report_type || !description) {
    return res.status(400).json({
      status: 'error',
      message: 'Missing required parameters: uid, report_type, description'
    });
  }

  // Validar report_type
  const validTypes = ['late_entry', 'early_departure', 'manual_report', 'multiple_entry_attempt'];
  if (!validTypes.includes(report_type)) {
    return res.status(400).json({
      status: 'error',
      message: 'Invalid report_type. Must be one of: ' + validTypes.join(', ')
    });
  }

  // Validar severity
  const validSeverities = ['info', 'warning', 'critical'];
  const reportSeverity = severity || 'warning';
  if (!validSeverities.includes(reportSeverity)) {
    return res.status(400).json({
      status: 'error',
      message: 'Invalid severity. Must be one of: ' + validSeverities.join(', ')
    });
  }

  // Buscar usuario por UID de tarjeta
  const { data: card, error: cardError } = await supabase
    .from('rfid_cards')
    .select('user_id')
    .eq('uid', uid)
    .single();

  if (cardError || !card) {
    return res.status(404).json({
      status: 'error',
      message: 'Card not found - cannot create report without user_id'
    });
  }

  // Crear reporte
  const { data: report, error: reportError } = await supabase
    .from('reports')
    .insert({
      user_id: card.user_id,
      sensor_id,
      report_type,
      severity: reportSeverity,
      description,
      metadata: metadata || {},
      timestamp: new Date().toISOString()
    })
    .select()
    .single();

  if (reportError) {
    console.error('Error creating report:', reportError);
    return res.status(500).json({
      status: 'error',
      message: 'Failed to create report'
    });
  }

  // Si es late_entry o early_departure, actualizar contador de strikes
  if (report_type === 'late_entry' || report_type === 'early_departure') {
    // Obtener threshold del metadata del usuario
    const { data: user } = await supabase
      .from('users')
      .select('metadata')
      .eq('id', card.user_id)
      .single();

    const threshold = user?.metadata?.strike_if?.[report_type] || 3;

    // Incrementar o crear strike
    const { data: existingStrike } = await supabase
      .from('user_strikes')
      .select('*')
      .eq('user_id', card.user_id)
      .eq('strike_type', report_type)
      .single();

    if (existingStrike) {
      await supabase
        .from('user_strikes')
        .update({
          count: existingStrike.count + 1,
          last_strike_at: new Date().toISOString(),
          updated_at: new Date().toISOString()
        })
        .eq('id', existingStrike.id);
    } else {
      await supabase
        .from('user_strikes')
        .insert({
          user_id: card.user_id,
          strike_type: report_type,
          count: 1,
          threshold,
          last_strike_at: new Date().toISOString()
        });
    }
  }

  res.json({
    status: 'ok',
    message: 'Report created successfully',
    report: {
      id: report.id,
      user_id: report.user_id,
      report_type: report.report_type,
      severity: report.severity,
      timestamp: report.timestamp
    }
  });
});

/**
 * 404 handler
 */
app.use((req, res) => {
  res.status(404).json({
    status: 'error',
    message: 'Endpoint not found'
  });
});

/**
 * Error handler
 */
app.use((err, req, res, next) => {
  console.error('Error:', err);
  res.status(500).json({
    status: 'error',
    message: 'Internal server error'
  });
});

// ============================================================================
// START SERVER
// ============================================================================

app.listen(PORT, () => {
  console.log('='.repeat(50));
  console.log('NeoAuth API Server');
  console.log('='.repeat(50));
  console.log(`🚀 Server running on port ${PORT}`);
  console.log(`📡 Environment: ${process.env.NODE_ENV || 'development'}`);
  console.log(`🔗 Supabase URL: ${process.env.SUPABASE_URL}`);
  console.log('='.repeat(50));
});
