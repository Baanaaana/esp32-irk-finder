# Installation Guide

[← Back to README](../README.md)

## Table of Contents
- [Using Prebuilt Firmware](#using-prebuilt-firmware)
  - [Web Installer (Recommended)](#web-installer-recommended)
  - [Manual Install with esptool](#manual-install-with-esptool)
- [Building from Source](#building-from-source)
  - [Installing PlatformIO](#installing-platformio)
  - [Building and Installing](#building-and-installing)

---

## Using Prebuilt Firmware

The easiest way to get started is using the prebuilt firmware binaries from the `releases` folder.

### Supported Devices
- **ESP32** - Original ESP32 chip
- **ESP32-S3** - Latest generation with USB support
- **ESP32-C3** - RISC-V based, cost-effective option

### Web Installer (Recommended)

The simplest method using ESP Web Tools:

1. Connect your ESP32, ESP32-S3, or ESP32-C3 to your computer via USB
2. Open Chrome or Edge browser (Firefox not supported)
3. Go to https://web.esphome.io/
4. Click "CONNECT" and select your device from the list
5. Click "INSTALL"
6. Choose "Choose File" and select the appropriate firmware:
   - **ESP32**: `releases/esp32-irk-finder.bin`
   - **ESP32-S3**: `releases/esp32-s3-irk-finder.bin`
   - **ESP32-C3**: `releases/esp32-c3-irk-finder.bin`
7. Click "INSTALL" and wait for the process to complete
8. Once done, click "CLOSE" and your device is ready!

### Manual Install with esptool

If you prefer command-line or the web installer doesn't work:

#### Prerequisites
Install esptool if not already installed:
```bash
pip install esptool
```

#### For ESP32
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
  --before default_reset --after hard_reset \
  write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x0 esp32-irk-finder.bin
```

#### For ESP32-S3
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
  --before default_reset --after hard_reset \
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB \
  0x0 esp32-s3-irk-finder.bin
```

#### For ESP32-C3
```bash
esptool.py --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 \
  --before default_reset --after hard_reset \
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB \
  0x0 esp32-c3-irk-finder.bin
```

**Port Selection:**
- **Windows**: `COM3`, `COM4`, etc. (check Device Manager)
- **macOS**: `/dev/cu.usbserial-0001`, `/dev/cu.SLAB_USBtoUART`, etc.
- **Linux**: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.

To find your port:
- **Windows**: Open Device Manager → Ports (COM & LPT)
- **macOS/Linux**: Run `ls /dev/tty*` or `ls /dev/cu.*`

---

## Building from Source

For developers who want to modify the firmware or build for custom configurations.

### Requirements
- ESP32, ESP32-S3, or ESP32-C3 development board
- USB cable (data cable, not charge-only)
- PlatformIO

### Installing PlatformIO

Choose one of these installation methods:

#### Option 1: VS Code Extension (Recommended)

Best for beginners and provides a full IDE experience:

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Open VS Code
3. Go to Extensions (Ctrl+Shift+X on Windows/Linux, Cmd+Shift+X on macOS)
4. Search for "PlatformIO IDE"
5. Click "Install" on the PlatformIO IDE extension
6. Restart VS Code when prompted
7. The PlatformIO icon will appear in the left sidebar

#### Option 2: Command Line Interface

For users who prefer terminal-based development:

**macOS (using Homebrew):**
```bash
brew install platformio
```

**Linux/macOS/Windows (using pip):**
```bash
# Ensure Python 3.6+ is installed
python3 -m pip install -U platformio
```

**Windows (using installer):**
Download and run the installer from [platformio.org](https://platformio.org/install/cli)

#### Option 3: PlatformIO Core (Standalone)

For advanced users who want to use PlatformIO with their own editor:

```bash
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py
```

### Building and Installing

#### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/esp32-irk-finder.git
cd esp32-irk-finder
```

#### 2. Open the Project

**VS Code Method:**
- File → Open Folder → Select the esp32-irk-finder folder
- PlatformIO will automatically detect the project

**CLI Method:**
- Navigate to the project directory in your terminal

#### 3. Select Target Board

The project supports multiple boards. Choose your target:

**VS Code Method:**
- Click PlatformIO icon in sidebar
- Select your environment (esp32dev, esp32s3, or esp32c3)

**CLI Method:**
- Use `-e` flag to specify environment:
  - `pio run -e esp32dev` for ESP32
  - `pio run -e esp32s3` for ESP32-S3
  - `pio run -e esp32c3` for ESP32-C3

#### 4. Build the Firmware

**VS Code Method:**
1. Click PlatformIO icon in sidebar
2. Expand your target environment
3. Click "Build"

**CLI Method:**
```bash
# For ESP32
pio run -e esp32dev

# For ESP32-S3
pio run -e esp32s3

# For ESP32-C3
pio run -e esp32c3
```

#### 5. Upload to Device

Connect your ESP32 via USB, then:

**VS Code Method:**
1. Click PlatformIO icon
2. Select "Upload" under your target environment

**CLI Method:**
```bash
# For ESP32
pio run -e esp32dev -t upload

# For ESP32-S3
pio run -e esp32s3 -t upload

# For ESP32-C3
pio run -e esp32c3 -t upload
```

#### 6. Monitor Serial Output

To see debug output and get the device's IP address:

**VS Code Method:**
- Click PlatformIO icon → "Monitor"

**CLI Method:**
```bash
pio device monitor
```

Serial monitor settings:
- Baud rate: 115200
- Line ending: LF
- Exit: Ctrl+C (CLI) or click Stop (VS Code)

### Troubleshooting Build Issues

**Upload fails:**
- Hold the BOOT button on your ESP32 while upload starts
- Try a different USB cable (must be data cable)
- Install USB drivers (CP210x or CH340 depending on your board)

**PlatformIO not found:**
- Restart your terminal/VS Code after installation
- Check PATH environment variable includes PlatformIO

**Build errors:**
- Delete `.pio` folder and rebuild
- Run `pio update` to update libraries
- Check you have the correct environment selected

---

## Next Steps

After installation:
1. [Configure WiFi and retrieve your first IRK](usage.md)
2. [Understand the API endpoints](api.md)
3. [Customize the configuration](configuration.md)

---

[← Back to README](../README.md) | [Usage Guide →](usage.md)