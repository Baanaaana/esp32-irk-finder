# ESP32 IRK Finder

A simple ESP32 firmware to retrieve iPhone's IRK (Identity Resolving Key) through Bluetooth pairing. The IRK can be used with Home Assistant, ESPresense, or other tracking systems to identify iPhones despite their rotating MAC addresses.

## Features

- BLE secure pairing with iPhone
- Web interface to display retrieved IRK
- Serial console output for debugging
- Support for both WiFi station and AP modes
- Real-time status updates via web interface
- Simple, focused functionality

## Requirements

- ESP32 development board
- PlatformIO (VS Code extension recommended)
- iPhone for testing

## Installation

1. Clone this repository:
```bash
git clone https://github.com/yourusername/esp32-irk-finder.git
cd esp32-irk-finder
```

2. Configure WiFi credentials in `include/config.h`:
```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

   Alternatively, use AP mode by setting:
```cpp
#define USE_AP_MODE true
```

3. Build and upload using PlatformIO:
```bash
pio run -t upload
```

4. Monitor serial output:
```bash
pio device monitor
```

## Usage

### Getting the iPhone IRK:

1. Power on the ESP32 and wait for it to initialize
2. Note the IP address from serial output or connect to AP mode (ESP32-IRK-Finder)
3. Open web browser and navigate to the ESP32's IP address
4. Click "Start New Pairing" button on the web interface
5. On your iPhone:
   - Go to Settings → Bluetooth
   - Find "ESP32_IRK_FINDER" and tap to connect
   - Confirm the passkey: **123456**
   - Accept the pairing request
6. The IRK will be displayed on both the web interface and serial console

### Web Interface

The web interface provides:
- Current IRK display (32 hex characters)
- Connected device MAC address
- Pairing status
- Button to start new pairing session
- Auto-refresh every 2 seconds

### API Endpoints

- `GET /` - Main web interface
- `GET /api/status` - JSON status (IRK, MAC, pairing status)
- `POST /api/start-pairing` - Start new pairing session

Example JSON response:
```json
{
  "irk": "1234567890ABCDEF1234567890ABCDEF",
  "mac": "AA:BB:CC:DD:EE:FF",
  "isPairing": false,
  "irkRetrieved": true
}
```

## Configuration Options

Edit `include/config.h` to customize:

- `WIFI_SSID` / `WIFI_PASSWORD` - WiFi credentials
- `USE_AP_MODE` - Use Access Point mode instead of connecting to WiFi
- `AP_SSID` / `AP_PASSWORD` - Access Point credentials
- `BLE_DEVICE_NAME` - Bluetooth device name
- `BLE_PASSKEY` - Pairing passkey (default: 123456)
- `LED_PIN` - Status LED pin (default: 2)

## Troubleshooting

- **Can't connect to WiFi**: Check credentials in config.h or use AP mode
- **iPhone doesn't show ESP32**: Ensure Bluetooth is enabled and try clicking "Start New Pairing"
- **IRK not retrieved**: Make sure to accept pairing and confirm the passkey
- **Web interface not accessible**: Check serial output for correct IP address

## Technical Details

- Uses NimBLE-Arduino library for better iOS compatibility
- AsyncWebServer for non-blocking web interface
- Implements secure BLE bonding with IRK exchange
- Stores IRK in memory (resets on power cycle)

## Security Note

The IRK is sensitive information that can be used to track your iPhone. Keep it secure and only share with trusted systems.

## License

MIT License - Feel free to modify and use as needed.