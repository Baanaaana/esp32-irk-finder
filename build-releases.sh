#!/bin/bash

# Script to build release firmware WITHOUT .env credentials
# This ensures release builds use only default values from config.h

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
cd "$PROJECT_DIR"

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}ESP32 IRK Finder - Release Builder${NC}"
echo "==========================================="
echo ""
echo -e "${YELLOW}Building release firmware WITHOUT .env credentials${NC}"
echo ""

# Temporarily rename .env file to prevent it from being loaded
if [ -f ".env" ]; then
    echo -e "${YELLOW}Temporarily disabling .env file...${NC}"
    mv .env .env.backup
    ENV_BACKUP=true
else
    ENV_BACKUP=false
fi

# Function to restore .env file
restore_env() {
    if [ "$ENV_BACKUP" = true ]; then
        echo -e "${YELLOW}Restoring .env file...${NC}"
        mv .env.backup .env
    fi
}

# Ensure .env is restored on exit
trap restore_env EXIT

# Clean previous builds
echo -e "${YELLOW}Cleaning previous builds...${NC}"
pio run -t clean
echo ""

# Build for all three variants
echo -e "${BLUE}Building ESP32 firmware...${NC}"
if ! pio run -e esp32dev; then
    echo -e "${RED}ESP32 build failed!${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}Building ESP32-S3 firmware...${NC}"
if ! pio run -e esp32s3; then
    echo -e "${RED}ESP32-S3 build failed!${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}Building ESP32-C3 firmware...${NC}"
if ! pio run -e esp32c3; then
    echo -e "${RED}ESP32-C3 build failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}All builds completed successfully!${NC}"
echo ""

# Create merged binaries
echo -e "${YELLOW}Creating merged binaries for web installer...${NC}"

# ESP32
esptool.py --chip esp32 merge_bin -o releases/irk-finder-esp32.bin \
    --flash_mode dio --flash_freq 40m --flash_size 4MB \
    0x1000 .pio/build/esp32dev/bootloader.bin \
    0x8000 .pio/build/esp32dev/partitions.bin \
    0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
    0x10000 .pio/build/esp32dev/firmware.bin \
    2>/dev/null

# ESP32-S3
esptool.py --chip esp32s3 merge_bin -o releases/irk-finder-esp32s3.bin \
    --flash_mode dio --flash_freq 80m --flash_size 8MB \
    0x0000 .pio/build/esp32s3/bootloader.bin \
    0x8000 .pio/build/esp32s3/partitions.bin \
    0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
    0x10000 .pio/build/esp32s3/firmware.bin \
    2>/dev/null

# ESP32-C3
esptool.py --chip esp32c3 merge_bin -o releases/irk-finder-esp32c3.bin \
    --flash_mode dio --flash_freq 80m --flash_size 4MB \
    0x0000 .pio/build/esp32c3/bootloader.bin \
    0x8000 .pio/build/esp32c3/partitions.bin \
    0xe000 ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
    0x10000 .pio/build/esp32c3/firmware.bin \
    2>/dev/null

echo ""
echo -e "${GREEN}════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Release build complete!${NC}"
echo -e "${GREEN}════════════════════════════════════════════${NC}"
echo ""
echo "Release binaries created in releases/:"
ls -lh releases/*.bin
echo ""
echo -e "${YELLOW}Note: These builds use DEFAULT credentials from config.h${NC}"
echo "Users will need to configure WiFi via the AP portal."
echo ""