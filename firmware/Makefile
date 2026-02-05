.PHONY: all build upload monitor clean cleanall run test erase d m f u uf c ca e t i up help

# Variables
PROJECT_DIR = $(shell pwd)
BUILD_PIO = ~/.platformio/penv/bin/platformio

# Comandos principales
all: build

# Acortaciones
d: deploy
f: flash
m: monitor
u: upload
uf: uploadfs
c: clean
ca: cleanall
e: erase
t: test
i: info
up: update

build:
	@echo "Compilando proyecto..."
	$(BUILD_PIO) run

upload:
	@echo "Cargando firmware al ESP32..."
	$(BUILD_PIO) run --target upload

uploadfs:
	@echo "Cargando sistema de archivos al ESP32..."
	$(BUILD_PIO) run --target uploadfs

monitor:
	@echo "Abriendo monitor serial..."
	$(BUILD_PIO) device monitor

run: upload monitor

# Compilar y cargar en un solo paso
flash: build uploadfs upload

# Compilar, cargar y monitorear
deploy: build uploadfs upload monitor

# Limpiar archivos de compilación
clean:
	@echo "Limpiando archivos de compilación..."
	$(BUILD_PIO) run --target clean

# Limpiar todo (incluyendo librerías)
cleanall:
	@echo "Limpiando todo..."
	$(BUILD_PIO) run --target cleanall
	rm -rf .pio

# Borrar la flash del ESP32
erase:
	@echo "Borrando flash del ESP32..."
	$(BUILD_PIO) run --target erase

# Ejecutar tests
test:
	@echo "Ejecutando tests..."
	$(BUILD_PIO) test

# Información del entorno
info:
	@echo "Información del proyecto:"
	@echo "  Directorio: $(PROJECT_DIR)"
	$(BUILD_PIO) system info

# Actualizar dependencias
update:
	@echo "Actualizando librerías y plataforma..."
	$(BUILD_PIO) lib update
	$(BUILD_PIO) platform update

# Ayuda
help:
	@echo "Makefile para RFID Auth Project"
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make build      - Compilar el proyecto"
	@echo "  make upload     - Cargar firmware al ESP32"
	@echo "  make monitor    - Abrir monitor serial"
	@echo "  make run        - Cargar y monitorear"
	@echo "  make flash      - Compilar y cargar"
	@echo "  make deploy     - Compilar, cargar y monitorear"
	@echo "  make clean      - Limpiar archivos de compilación"
	@echo "  make fullclean  - Limpiar todo (incluye librerías)"
	@echo "  make erase      - Borrar flash del ESP32"
	@echo "  make test       - Ejecutar tests"
	@echo "  make info       - Mostrar información del sistema"
	@echo "  make update     - Actualizar dependencias"
	@echo "  make help       - Mostrar esta ayuda"
