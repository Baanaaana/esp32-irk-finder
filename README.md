# ESP32 IRK Finder

A simple ESP32 firmware to retrieve iPhone's IRK (Identity Resolving Key) through Bluetooth pairing. The IRK can be used with Home Assistant, ESPresense, or other tracking systems to identify iPhones despite their rotating MAC addresses.

![ESP32 IRK Finder Web Interface](esp32-irk-finder-webserver.jpg)

## Quick Start - Using Prebuilt Firmware

### Web Installer (Recommended)
1. Connect your ESP32 to your computer via USB
2. Open Chrome or Edge browser
3. Go to https://web.esphome.io/
4. Click "CONNECT" and select your ESP32
5. Click "INSTALL"
6. Choose "Choose File" and select `releases/esp32-irk-finder.bin`
7. Click "INSTALL" and wait for completion

### Manual Install with esptool
1. Download `esp32-irk-finder.bin` from the `releases` folder
2. Install esptool (if not already installed):
   ```bash
   pip install esptool
   ```
3. Flash the firmware:
   ```bash
   esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 \
     write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
     0x0 esp32-irk-finder.bin
   ```
   Replace `/dev/ttyUSB0` with your ESP32's port (e.g., `COM3` on Windows, `/dev/cu.usbserial-0001` on macOS)

## Features

- BLE secure pairing with iPhone
- Web interface to display retrieved IRK
- Serial console output for debugging
- Support for both WiFi station and AP modes
- Real-time status updates via web interface
- Simple, focused functionality

## Requirements

- ESP32 development board
- iPhone for testing
- PlatformIO (only needed if building from source)

## Building from Source

### Installing PlatformIO

PlatformIO is required to build the firmware from source. Choose one of these installation methods:

### Option 1: VS Code Extension (Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Open VS Code and go to Extensions (Ctrl+Shift+X / Cmd+Shift+X)
3. Search for "PlatformIO IDE"
4. Click Install on the PlatformIO IDE extension
5. Restart VS Code when prompted
6. The PlatformIO icon will appear in the left sidebar

### Option 2: Command Line Interface

#### macOS (using Homebrew):
```bash
brew install platformio
```

#### Linux/macOS/Windows (using pip):
```bash
# Ensure Python 3.6+ is installed
python3 -m pip install -U platformio
```

#### Windows (using installer):
Download and run the installer from [platformio.org](https://platformio.org/install/cli)

### Option 3: PlatformIO Core (Standalone)

For advanced users who prefer using PlatformIO with their own editor:
```bash
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py
```

### Building and Installing

1. Clone this repository:
```bash
git clone https://github.com/yourusername/esp32-irk-finder.git
cd esp32-irk-finder
```

2. Open the project:
   - **VS Code**: File → Open Folder → Select the esp32-irk-finder folder
   - **CLI**: Navigate to the project directory in terminal

3. Build and upload the firmware:

   **VS Code Method:**
   - Click PlatformIO icon in sidebar
   - Select "Upload" in the Project Tasks

   **CLI Method:**
   ```bash
   pio run -t upload
   ```

4. Monitor serial output to see the ESP32's IP address:

   **VS Code Method:**
   - Click PlatformIO icon → "Monitor"

   **CLI Method:**
   ```bash
   pio device monitor
   ```

## Usage

### Initial Setup - WiFi Configuration:

On first use, the ESP32 will start in Access Point (AP) mode:

1. Connect to the ESP32's WiFi network:
   - SSID: `ESP32-IRK-FINDER`
   - Password: `12345678`

2. Open your browser and navigate to `http://192.168.4.1`
   - You'll be automatically redirected to the WiFi configuration page

3. Select your WiFi network from the list or enter SSID manually

4. Enter your WiFi password and click "Save & Connect"

5. The ESP32 will restart and connect to your WiFi network

Note: You can still retrieve IRKs while in AP mode by navigating directly to the IRK finder page

### Getting the iPhone IRK:

1. Power on the ESP32 and wait for it to initialize
2. If connected to WiFi, note the IP address from serial output
   - Or connect via AP mode: `192.168.4.1`
3. Open web browser and navigate to the ESP32's IP address
4. On your iPhone, download a BLE scanner app:
   - LightBlue
   - nRF Connect
   - Bluetooth Terminal
   - BLE Scanner
5. Open the BLE scanner app and scan for devices
6. Find "ESP32_IRK_FINDER" and tap to connect
7. When prompted for pairing, accept the request
8. Enter passkey: **123456**
9. The IRK will be displayed on the web interface in multiple formats:
   - Standard Hex
   - ESPresense (reversed)
   - Base64 (for Home Assistant Private BLE)
   - Hex Array

### Web Interface Features

The web interface provides:
- IRK display in 4 different formats (Hex, ESPresense, Base64, Array)
- Connected device MAC address
- Copy buttons for each IRK format
- WiFi configuration portal
- Network scanning and selection
- Auto-refresh every 2 seconds
- Clean, modern shadcn-style UI

### API Endpoints

- `GET /` - Main IRK finder interface
- `GET /wifi` - WiFi configuration page
- `GET /api/status` - JSON status with all IRK formats
- `GET /api/wifi/scan` - Scan for WiFi networks
- `POST /api/wifi/save` - Save WiFi credentials
- `POST /api/wifi/clear` - Clear saved WiFi credentials
- `GET /api/wifi/status` - Current WiFi connection status

Example status response:
```json
{
  "irk": "112233445566778899AABBCCDDEEFF00",
  "irkReversed": "00FFEEDDCCBBAA998877665544332211",
  "irkBase64": "ESIzRFVmd4iZqrvM3e7/AA==",
  "irkArray": "0x11,0x22,0x33,0x44,0x55,0x66,...",
  "mac": "AA:BB:CC:DD:EE:FF",
  "irkRetrieved": true,
  "isAPMode": false,
  "ipAddress": "192.168.1.100"
}
```

## Configuration Options

Edit `include/config.h` to customize defaults:

- `AP_SSID` - Access Point name (default: "ESP32-IRK-FINDER")
- `AP_PASSWORD` - Access Point password (default: "12345678")
- `BLE_DEVICE_NAME` - Bluetooth device name (default: "ESP32_IRK_FINDER")
- `BLE_PASSKEY` - Pairing passkey (default: 123456)
- `LED_PIN` - Status LED pin (default: 2)
- `WEB_SERVER_PORT` - Web server port (default: 80)

Note: WiFi credentials are now configured through the web interface and stored persistently.

## Troubleshooting

- **Can't connect to WiFi**: Use the web configuration portal in AP mode
- **iPhone doesn't show ESP32**: The device only appears in BLE scanner apps, not in iPhone Settings
- **IRK not retrieved**: Make sure to accept pairing and confirm the passkey (123456)
- **Web interface not accessible**: Check serial output for correct IP address
- **WiFi credentials lost**: Clear credentials via web interface and reconfigure

## Technical Details

- Uses ESP-IDF Bluedroid stack for reliable iOS compatibility
- AsyncWebServer for non-blocking web interface
- Implements secure BLE bonding with IRK exchange
- Preferences library for persistent WiFi credential storage
- Automatic AP mode fallback when WiFi connection fails
- IRK displayed in multiple formats for various integrations
- Bonds are cleared on each startup for fresh pairing

## Security Note

The IRK is sensitive information that can be used to track your iPhone. Keep it secure and only share with trusted systems.

## License

MIT License - Feel free to modify and use as needed.