# ESP32 IRK Finder - Prebuilt Firmware

This folder contains prebuilt firmware binaries for ESP32 and ESP32-S3 devices.

## Files

- `esp32-irk-finder.bin` - Complete firmware for ESP32 (includes bootloader, partitions, and application)
- `esp32-s3-irk-finder.bin` - Complete firmware for ESP32-S3 (includes bootloader, partitions, and application)

## Flashing Instructions

### Option 1: Web-based Installer (Easiest)

1. Open Chrome or Edge browser
2. Go to https://web.esphome.io/
3. Connect your ESP32 or ESP32-S3 via USB
4. Click "CONNECT"
5. Select your device from the list
6. Click "INSTALL"
7. Choose "Choose File" and select:
   - **ESP32**: `esp32-irk-finder.bin`
   - **ESP32-S3**: `esp32-s3-irk-finder.bin`
8. Click "INSTALL" and wait for the process to complete

### Option 2: Using esptool

#### For ESP32:
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  --before default_reset --after hard_reset \
  write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x0 esp32-irk-finder.bin
```

#### For ESP32-S3:
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
  --before default_reset --after hard_reset \
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 esp32-s3-irk-finder.bin
```

Replace `/dev/ttyUSB0` with your actual port:
- **Windows**: `COM3`, `COM4`, etc.
- **macOS**: `/dev/cu.usbserial-0001`, `/dev/cu.SLAB_USBtoUART`, etc.
- **Linux**: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.

## Troubleshooting

### Upload fails
- Hold the BOOT button on your ESP32 while the upload starts
- Try a different USB cable (data cable, not charge-only)
- Install CP210x or CH340 drivers if needed

### Port not found
- Check device manager (Windows) or `ls /dev/tty*` (macOS/Linux)
- Install appropriate USB-to-Serial drivers for your ESP32 board

### After Flashing

1. Open a serial monitor at 115200 baud to see the device's IP address
2. Connect to the ESP32's WiFi network:
   - SSID: `ESP32-IRK-FINDER`
   - Password: `12345678`
3. You'll be automatically redirected to configure your WiFi
4. Follow the instructions in the main README to retrieve iPhone IRKs

## Version History

- **v1.0.0** - Initial release with BLE IRK retrieval, WiFi configuration portal, and web interface