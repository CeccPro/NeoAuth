.PHONY: d f m u uf c ca e t i up

# Variables
PROJECT_DIR = $(shell pwd)
BUILD_PIO = ~/.platformio/penv/bin/platformio
DEVICE = /dev/ttyUSB0

# Nombres cortos
d:
	@cd $(PROJECT_DIR)/firmware && make d DEVICE=$(DEVICE)

f:
	@cd $(PROJECT_DIR)/firmware && make f DEVICE=$(DEVICE)

m:
	@cd $(PROJECT_DIR)/firmware && make m DEVICE=$(DEVICE)

u: 
	@cd $(PROJECT_DIR)/firmware && make u DEVICE=$(DEVICE)

uf:
	@cd $(PROJECT_DIR)/firmware && make uf DEVICE=$(DEVICE)

c: 
	@cd $(PROJECT_DIR)/firmware && make c DEVICE=$(DEVICE)

ca: 
	@cd $(PROJECT_DIR)/firmware && make ca DEVICE=$(DEVICE)
e:
	@cd $(PROJECT_DIR)/firmware && make e DEVICE=$(DEVICE)

t:
	@cd $(PROJECT_DIR)/firmware && make t DEVICE=$(DEVICE)

i:
	@cd $(PROJECT_DIR)/firmware && make i DEVICE=$(DEVICE)

up:
	@cd $(PROJECT_DIR)/firmware && make up DEVICE=$(DEVICE)

com: 
	@git add .
	@git commit -m "Update"
	@git push

# Nombres largos
deploy: d
flash: f
monitor: m
upload: u
uploadfs: uf
clean: c
cleanall: ca
erase: e
test: t
info: i
update: up
commit: com

help:
	@echo "Comandos disponibles:"
	@echo "  d   - Deploy (compilar, cargar y monitorear)"
	@echo "  f   - Flash (compilar y cargar)"
	@echo "  m   - Monitor (abrir monitor serial)"
	@echo "  u   - Upload (cargar firmware)"
	@echo "  uf  - UploadFS (cargar sistema de archivos)"
	@echo "  c   - Clean (limpiar archivos de compilación)"
	@echo "  ca  - Clean All (limpiar todo, incluyendo librerías)"
	@echo "  e   - Erase (borrar flash del ESP32)"
	@echo "  t   - Test (ejecutar pruebas)"
	@echo "  i   - Info (mostrar información del proyecto)"
	@echo "  up  - Update (actualizar PlatformIO y dependencias)"