# ESP32 IRK Finder - Prebuilt Firmware

This folder contains the prebuilt firmware binary ready to flash to your ESP32.

## File

- `esp32-irk-finder.bin` - Complete firmware image (includes bootloader, partitions, and application)

## Flashing Instructions

### Option 1: Web-based Installer (Easiest)

1. Open Chrome or Edge browser
2. Go to https://web.esphome.io/
3. Connect your ESP32 via USB
4. Click "CONNECT"
5. Select your ESP32 device from the list
6. Click "INSTALL"
7. Choose "Choose File" and select `esp32-irk-finder.bin`
8. Click "INSTALL" and wait for the process to complete

### Option 2: Using esptool

```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  --before default_reset --after hard_reset \
  write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x0 esp32-irk-finder.bin
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