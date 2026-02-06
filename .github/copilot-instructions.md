# NeoAuth - AI Coding Agent Instructions

## Project Overview

**NeoAuth** is an IoT-based RFID access control system with two main components:
- **Firmware** (ESP32): C++ embedded code for RFID readers with multiple operational modes
- **Server** (Node.js): Express API backend with Supabase integration for user/card management

## Architecture

### Dual-Component System
```
┌─────────────────┐                    ┌──────────────────┐
│   ESP32 Device  │ ←──HTTP/I2C──────→ │   Node.js API    │
│  (firmware/)    │                    │   (server/)      │
│  - RFID reader  │                    │  - Supabase DB   │
│  - WiFi AP      │                    │  - Auth tokens   │
│  - Web UI       │                    │  - Access logs   │
└─────────────────┘                    └──────────────────┘
```

### Operating Modes (firmware/lib/mode_manager/)
- **MODE_STANDALONE**: RFID identification only, queries API to identify users
- **MODE_TURNSTILE**: Physical access control via I2C slave device (lock/unlock)
- **MODE_REGISTRATION**: Register new RFID cards
- **MODE_SYNC**: Backend synchronization

Modes are persisted in `/data/config.json` (SPIFFS filesystem).

### Task-Based Architecture (firmware/src/main.cpp)
ESP32 uses **non-blocking task scheduler** to prevent watchdog resets:
```cpp
Task tasks[TASK_COUNT] = {
  {TASK_WIFI_PERIODIC,        0, 30000, true},
  {TASK_RFID_CHECK_CARD,       0, 50,    true},
  {TASK_TURNSTILE_PERIODIC,    0, 100,   false},
  // ...
};
```
Each task runs at specified intervals with `yield()` between executions. When adding features, integrate into task system rather than blocking `loop()`.

### I2C Communication Pattern
Master (ESP32) communicates with slave devices (Arduino Nano) for turnstile control:
- **Master**: `firmware/lib/i2c_comm/` - sends commands (UNLOCK/LOCK/STATUS)
- **Slave examples**: `firmware/examples/slave_turnstile/` - hardware control implementations
- Pin configuration: SDA=26, SCL=25, Address=0x08

## Development Workflows

### Firmware Development
```bash
# Root Makefile shortcuts (delegates to firmware/Makefile)
make d        # deploy: build + uploadfs + upload + monitor
make f        # flash: build + uploadfs + upload
make u        # upload: firmware only
make uf       # uploadfs: SPIFFS filesystem only
make m        # monitor: serial output
make c        # clean build
```

**CRITICAL**: Always use `make uf` or `make f` when modifying files in `firmware/data/` (web UI, config.json).

Set custom device: `make f DEVICE=/dev/ttyUSB1`

### Server Development
```bash
cd server
npm install
npm run dev     # Development with --watch
npm start       # Production
```

### Deployment
- **Server**: Nixpacks configuration in root `nixpacks.toml` (Railway deployment)
- **Database**: Supabase migrations in `server/sql_migrations/`

## Key Conventions

### Authentication Pattern (server/src/index.js)
API uses **HMAC-SHA256 token authentication**:
```javascript
// Token = HMAC-SHA256(AUTH_SECRET + sensor_id)
const token = crypto.createHmac('sha256', AUTH_SECRET)
  .update(sensorId)
  .digest('hex');
```

All endpoints require `sensor_id` + `auth_token` in request body. `AUTH_SECRET` defined in `firmware/include/config.h` and server `.env`.

### Configuration Management
- **Firmware config**: `firmware/data/config.json` (mode, WiFi, API settings)
- **Config loader**: `firmware/lib/config_manager/` - loads from SPIFFS
- **Server config**: `.env` file (Supabase keys, AUTH_SECRET)

### WebSocket Communication (firmware/lib/web_server/)
ESP32 serves web UI at `http://Sensor_RFID_AP/` (AP mode) or device IP. 
WebSocket broadcasts real-time events:
- WiFi connection status
- RFID card detections
- Mode changes
- Turnstile state updates

### Library Structure Pattern
Each firmware library (`firmware/lib/`) follows structure:
```
library_name/
  ├── library_name.h    # Class declaration
  ├── library_name.cpp  # Implementation
```

Classes initialized as globals in `main.cpp`, passed as pointers to dependent modules.

## API Endpoints

### POST /api/v1/access
Validate RFID card for turnstile mode (grants/denies access).

### POST /api/v1/who_is  
Identify user by RFID UID for standalone mode.

### POST /api/v1/heartbeat
Sensor health check (updates `last_heartbeat` in DB).

All require authentication via `sensor_id` + `auth_token`.

## Database Schema (Supabase)
- **sensors**: RFID devices, modes, heartbeats
- **users**: User profiles with active status
- **rfid_cards**: UID→User mapping
- **access_logs**: All access attempts with metadata

See `server/sql_migrations/20260205_001_initial_schema.sql` for complete schema.

## Common Gotchas

1. **Watchdog Resets**: Never use blocking delays in firmware loop. Use task system with `yield()`.
2. **SPIFFS Updates**: Web UI changes require `make uf` to flash filesystem.
3. **Mode Dependencies**: Turnstile mode requires I2C slave device. Standalone mode works independently.
4. **WiFi Callbacks**: Use `wifiManager.setOnConnectedCallback()` to notify web clients of connection changes.
5. **PlatformIO Paths**: Build system uses `~/.platformio/penv/bin/platformio` (hardcoded in Makefiles).

## File References
- Core firmware: [main.cpp](../firmware/src/main.cpp)
- API server: [index.js](../server/src/index.js)
- Configuration: [config.h](../firmware/include/config.h), [config.json](../firmware/data/config.json)
- Build commands: [Makefile](../Makefile), [firmware/Makefile](../firmware/Makefile)
- Web UI: [firmware/data/index.html](../firmware/data/index.html)