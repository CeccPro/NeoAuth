# Protocolo de Comunicación Torniquete

## Formato de Mensajes

Todos los mensajes tienen **4 bytes**:

```
[BYTE 0: CMD] [BYTE 1: PARAM_HIGH] [BYTE 2: PARAM_LOW] [BYTE 3: RESERVED]
```

- **CMD**: Código del comando (1 byte)
- **PARAM**: Parámetro de 16 bits (2 bytes, big endian)
- **RESERVED**: Reservado para uso futuro (1 byte)

## Comandos Disponibles

### 0x01 - UNLOCK (Desbloquear)

Desbloquea el torniquete por un tiempo específico o indefinidamente.

**Formato:**
```
0x01 [SECONDS_HIGH] [SECONDS_LOW] 0x00
```

**Parámetro (SECONDS):**
- `0x0000` (0): Desbloqueo **indefinido** (no se auto-bloquea)
- `0x0001` - `0xFFFF` (1-65535): Segundos antes de auto-bloquear

**Ejemplos:**

```cpp
// Desbloquear por 5 segundos
uint8_t msg[] = {0x01, 0x00, 0x05, 0x00};

// Desbloquear por 30 segundos
uint8_t msg[] = {0x01, 0x00, 0x1E, 0x00};

// Desbloquear indefinidamente
uint8_t msg[] = {0x01, 0x00, 0x00, 0x00};

// Desbloquear por 1 hora (3600 segundos = 0x0E10)
uint8_t msg[] = {0x01, 0x0E, 0x10, 0x00};
```

### 0x02 - LOCK (Bloquear)

Bloquea el torniquete inmediatamente, cancelando cualquier auto-bloqueo pendiente.

**Formato:**
```
0x02 0x00 0x00 0x00
```

**Parámetro:** Ignorado (enviar 0x0000)

### 0x03 - STATUS (Consultar Estado)

Solicita el estado actual del torniquete.

**Formato:**
```
0x03 0x00 0x00 0x00
```

**Respuesta del Slave:**
```
[LOCKED] [RESERVED] [RESERVED] [RESERVED]
```
- `LOCKED`: 0x01 = bloqueado, 0x00 = desbloqueado

### 0x04 - PING (Verificar Conectividad)

Test de conectividad con el slave.

**Formato:**
```
0x04 0x00 0x00 0x00
```

**Respuesta del Slave:**
```
0xAA 0x55 [LOCKED] 0x00
```

## Uso desde el ESP32

### Función Básica (usa autoLockDelay configurado)

```cpp
turnstileMode.unlockTurnstile();  // Usa el delay configurado
```

### Función con Parámetro

```cpp
// Desbloquear por 10 segundos
turnstileMode.unlockTurnstile(10);

// Desbloquear por 2 minutos
turnstileMode.unlockTurnstile(120);

// Desbloquear indefinidamente
turnstileMode.unlockTurnstile(0);
```

## Uso desde WebSocket

### Desbloquear con Default

```javascript
ws.send(JSON.stringify({
  command: "unlockTurnstile"
}));
// Usa el autoLockDelay configurado del ESP32
```

### Desbloquear con Duración Específica

```javascript
// Desbloquear por 15 segundos
ws.send(JSON.stringify({
  command: "unlockTurnstile",
  duration: 15
}));

// Desbloquear indefinidamente (valor especial)
ws.send(JSON.stringify({
  command: "unlockTurnstile",
  duration: 9999
}));
```

### Bloquear

```javascript
ws.send(JSON.stringify({
  command: "lockTurnstile"
}));
```

## Comportamiento del Slave (Arduino)

### Variables de Estado

```cpp
bool locked = true;              // Estado actual
unsigned long unlockTime = 0;    // Timestamp de desbloqueo
unsigned long autoLockDelay = 0; // Delay en ms (0 = indefinido)
```

### Lógica de Auto-Bloqueo

1. Al recibir **UNLOCK**:
   - Si parámetro = 0 → `autoLockDelay = 0` (indefinido)
   - Si parámetro > 0 → `autoLockDelay = param * 1000` (convertir a ms)
   - Guardar `unlockTime = millis()`

2. En el `loop()`:
   - Si `autoLockDelay > 0` y tiempo transcurrido ≥ `autoLockDelay`
   - → Bloquear automáticamente

3. Al recibir **LOCK**:
   - Bloquear inmediatamente
   - Resetear `autoLockDelay = 0`

## Ejemplos de Uso

### Caso 1: Acceso Normal (5 segundos)

```cpp
// ESP32 detecta tarjeta autorizada
turnstileMode.unlockTurnstile(5);

// Slave recibe: [0x01, 0x00, 0x05, 0x00]
// Slave desbloquea por 5 segundos
// Después de 5s → auto-bloquea
```

### Caso 2: Modo Mantenimiento (indefinido)

```cpp
// Desde interfaz web o comando
turnstileMode.unlockTurnstile(0);

// Slave recibe: [0x01, 0x00, 0x00, 0x00]
// Slave desbloquea indefinidamente
// NO se auto-bloquea hasta recibir LOCK manual
```

### Caso 3: Evento Especial (30 minutos)

```cpp
// Desbloquear por 30 minutos = 1800 segundos
turnstileMode.unlockTurnstile(1800);

// Slave recibe: [0x01, 0x07, 0x08, 0x00]
// (0x0708 = 1800 en hexadecimal)
// Después de 30 min → auto-bloquea
```

### Caso 4: Emergencia (bloqueo manual)

```cpp
// Bloquear inmediatamente sin importar el estado
turnstileMode.lockTurnstile();

// Slave recibe: [0x02, 0x00, 0x00, 0x00]
// Cancela cualquier auto-lock pendiente
```

## Límites y Consideraciones

- **Rango de tiempo**: 1 a 65535 segundos (~18 horas)
- **Valor especial 0**: Desbloqueo indefinido
- **Overflow de millis()**: El slave maneja el overflow de `millis()` correctamente
- **Precisión**: ±10ms debido al `delay(10)` en el loop del slave

## Diagrama de Flujo

```
┌─────────────┐
│ Tarjeta OK  │
└──────┬──────┘
       │
       v
┌──────────────────────────────┐
│ turnstileMode.unlockTurnstile│
│        (duration)             │
└──────┬───────────────────────┘
       │
       v
┌──────────────────────────────┐
│ sideconn.sendMessage()        │
│ [0x01, HIGH, LOW, 0x00]       │
└──────┬───────────────────────┘
       │ (I2C/SPI/Serial)
       v
┌──────────────────────────────┐
│ Slave: receiveEvent()         │
│ - Lee comando + parámetro     │
│ - Configura autoLockDelay     │
│ - Desbloquea hardware         │
└──────┬───────────────────────┘
       │
       v
┌──────────────────────────────┐
│ Slave: loop()                 │
│ - Monitorea tiempo            │
│ - Auto-bloquea si expira      │
└───────────────────────────────┘
```

## Compatibilidad

Este protocolo es compatible con:
- ✅ Arduino Nano/Uno (I2C slave)
- ✅ ESP32 (I2C master via i2c_comm)
- ✅ ESP32 (SPI master via sideconn)
- ✅ Cualquier MCU con I2C/SPI/Serial

Para usar con I2C, adaptar el código de `sideconn` para usar la librería `i2c_comm`.
