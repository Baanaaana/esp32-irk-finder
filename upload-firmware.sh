#!/bin/bash

# Script to upload ESP32 IRK Finder firmware via USB
# Usage: ./upload-firmware.sh [board_type]
#
# board_type can be: esp32, esp32s3, or esp32c3
# If no board type is specified, the script will try to auto-detect

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
cd "$PROJECT_DIR"

# Color codes
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}ESP32 IRK Finder Firmware Uploader${NC}"
echo "==========================================="
echo ""

# Get board type from argument or prompt user
if [ -z "$1" ]; then
    echo -e "${YELLOW}Select ESP32 variant:${NC}"
    echo "1) ESP32 (standard)"
    echo "2) ESP32-S3"
    echo "3) ESP32-C3"
    echo ""
    read -p "Enter choice [1-3]: " choice

    case $choice in
        1)
            ENV_NAME="esp32dev"
            BOARD_NAME="ESP32"
            ;;
        2)
            ENV_NAME="esp32s3"
            BOARD_NAME="ESP32-S3"
            ;;
        3)
            ENV_NAME="esp32c3"
            BOARD_NAME="ESP32-C3"
            ;;
        *)
            echo -e "${RED}Invalid choice!${NC}"
            exit 1
            ;;
    esac
else
    case $1 in
        esp32|ESP32)
            ENV_NAME="esp32dev"
            BOARD_NAME="ESP32"
            ;;
        esp32s3|ESP32S3|ESP32-S3)
            ENV_NAME="esp32s3"
            BOARD_NAME="ESP32-S3"
            ;;
        esp32c3|ESP32C3|ESP32-C3)
            ENV_NAME="esp32c3"
            BOARD_NAME="ESP32-C3"
            ;;
        *)
            echo -e "${RED}Invalid board type: $1${NC}"
            echo "Valid options: esp32, esp32s3, esp32c3"
            exit 1
            ;;
    esac
fi

echo -e "${GREEN}Selected board: $BOARD_NAME${NC}"
echo ""

# Check if device is connected
echo -e "${YELLOW}Checking for connected $BOARD_NAME device...${NC}"

# Look for typical ESP32 USB ports
PORT=""
if ls /dev/cu.usbserial* >/dev/null 2>&1; then
    PORT=$(ls /dev/cu.usbserial* | head -1)
    echo -e "${GREEN}✓ Found device on macOS: $PORT${NC}"
elif ls /dev/cu.wchusbserial* >/dev/null 2>&1; then
    PORT=$(ls /dev/cu.wchusbserial* | head -1)
    echo -e "${GREEN}✓ Found device on macOS (WCH): $PORT${NC}"
elif ls /dev/cu.SLAB_USBtoUART* >/dev/null 2>&1; then
    PORT=$(ls /dev/cu.SLAB_USBtoUART* | head -1)
    echo -e "${GREEN}✓ Found device on macOS (Silicon Labs): $PORT${NC}"
elif ls /dev/cu.usbmodem* >/dev/null 2>&1; then
    PORT=$(ls /dev/cu.usbmodem* | head -1)
    echo -e "${GREEN}✓ Found device on macOS (USB modem): $PORT${NC}"
elif ls /dev/ttyUSB* >/dev/null 2>&1; then
    PORT=$(ls /dev/ttyUSB* | head -1)
    echo -e "${GREEN}✓ Found device on Linux: $PORT${NC}"
elif ls /dev/ttyACM* >/dev/null 2>&1; then
    PORT=$(ls /dev/ttyACM* | head -1)
    echo -e "${GREEN}✓ Found device on Linux (ACM): $PORT${NC}"
else
    echo -e "${RED}✗ No $BOARD_NAME device detected!${NC}"
    echo ""
    echo "Please ensure:"
    echo "1. The $BOARD_NAME is connected via USB"
    echo "2. The device is powered on"
    echo "3. USB drivers are installed"
    echo ""
    echo "If using macOS, you may need to install:"
    echo "  - CP210x drivers (for most ESP32 boards)"
    echo "  - CH340/CH341 drivers (for some clone boards)"
    echo ""
    echo "If using Linux, ensure you have access to serial ports:"
    echo "  sudo usermod -a -G dialout $USER"
    echo "  (logout and login again after running this command)"
    echo ""

    # Allow manual port entry
    read -p "Enter serial port manually (or press Enter to exit): " MANUAL_PORT
    if [ -z "$MANUAL_PORT" ]; then
        exit 1
    fi
    PORT=$MANUAL_PORT
fi

echo ""

# Check if .env file exists
if [ ! -f ".env" ]; then
    echo -e "${YELLOW}Warning: .env file not found!${NC}"
    echo "WiFi credentials will use default values from config.h"
    echo ""
    echo "To set WiFi credentials, create a .env file with:"
    echo "  WIFI_SSID=YourWiFiName"
    echo "  WIFI_PASSWORD=YourWiFiPassword"
    echo ""
    read -p "Continue with default values? [y/N]: " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Aborted."
        exit 1
    fi
fi

echo -e "${YELLOW}Building and uploading firmware...${NC}"
echo "Environment: $ENV_NAME"
echo "Port: $PORT"
echo ""

# Build and upload firmware
if pio run -e $ENV_NAME --target upload --upload-port $PORT; then
    echo ""
    echo -e "${GREEN}════════════════════════════════════════════${NC}"
    echo -e "${GREEN}✓ Firmware uploaded successfully!${NC}"
    echo -e "${GREEN}════════════════════════════════════════════${NC}"
    echo ""
    echo "The $BOARD_NAME IRK Finder has been programmed."
    echo ""
    echo -e "${BLUE}Next steps:${NC}"
    echo "1. The device will restart automatically"
    echo "2. Connect to WiFi network 'ESP32-IRK-FINDER' (password: 12345678)"
    echo "3. Open http://192.168.4.1 in your browser"
    echo "4. Configure your WiFi settings"
    echo "5. After WiFi setup, access device at:"
    echo "   - http://esp32-irk-finder.local"
    echo "   - Or check serial monitor for IP address"
    echo ""
    echo -e "${BLUE}To retrieve iPhone IRK:${NC}"
    echo "1. Open Bluetooth settings on your iPhone"
    echo "2. Pair with 'ESP32_IRK_FINDER'"
    echo "3. Enter passkey: 123456"
    echo "4. The IRK will appear on the web interface"
    echo ""
    echo "To monitor serial output:"
    echo "  pio device monitor -p $PORT -b 115200"
    echo ""

    # Offer to open serial monitor
    read -p "Open serial monitor now? [y/N]: " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}Opening serial monitor... (Press Ctrl+C to exit)${NC}"
        pio device monitor -p $PORT -b 115200
    fi
else
    echo ""
    echo -e "${RED}✗ Upload failed!${NC}"
    echo ""
    echo "Troubleshooting:"
    echo "1. Check USB connection"
    echo "2. Try pressing the BOOT/IO0 button during upload"
    echo "3. Check if another program is using the serial port"
    echo "4. Verify the device is a $BOARD_NAME"
    echo "5. Try a different USB cable (some are charge-only)"
    echo ""

    if [ "$BOARD_NAME" == "ESP32-C3" ] || [ "$BOARD_NAME" == "ESP32-S3" ]; then
        echo "Note: $BOARD_NAME boards with native USB might need:"
        echo "  - Hold BOOT button while plugging in USB"
        echo "  - Or hold BOOT and press RESET, then release both"
    fi

    exit 1
fi