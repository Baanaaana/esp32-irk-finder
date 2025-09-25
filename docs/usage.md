# Usage Guide

[← Back to README](../README.md)

## Table of Contents
- [Initial WiFi Setup](#initial-wifi-setup)
- [Retrieving iPhone IRK](#retrieving-iphone-irk)
- [IRK Format Explanations](#irk-format-explanations)
- [Troubleshooting](#troubleshooting)

---

## Initial WiFi Setup

On first use, the ESP32 will start in Access Point (AP) mode for WiFi configuration.

### Connecting to the Configuration Portal

1. **Connect to ESP32's WiFi Network**
   - SSID: `ESP32-IRK-FINDER`
   - Password: `12345678`

2. **Access Configuration Portal**
   - Open your browser and navigate to `http://192.168.4.1`
   - You'll be automatically redirected to the WiFi configuration page

3. **Configure Your WiFi**
   - Select your WiFi network from the list or enter SSID manually
   - Enter your WiFi password
   - Click "Save & Connect"

4. **Device Restart**
   - The ESP32 will restart and connect to your WiFi network
   - Check serial monitor (115200 baud) to see the assigned IP address

**Note:** You can still retrieve IRKs while in AP mode by navigating directly to the IRK finder page at `http://192.168.4.1`

---

## Retrieving iPhone IRK

### Prerequisites
- ESP32 powered on and initialized
- BLE scanner app on your iPhone (see below)
- Know the device's IP address or use AP mode

### Step-by-Step Instructions

1. **Access the Web Interface**
   - If connected to WiFi: Open browser and go to `http://esp32-irk-finder.local`
   - Alternative: Use the ESP32's IP address (shown in serial monitor)
   - If in AP mode: Connect to ESP32 WiFi and go to `http://192.168.4.1` or `http://esp32-irk-finder.local`

2. **Install a BLE Scanner App**

   Download one of these apps from the App Store:
   - **LightBlue** - Popular and reliable
   - **nRF Connect** - Professional tool from Nordic
   - **Bluetooth Terminal** - Simple interface
   - **BLE Scanner** - Basic functionality

   **Important:** The ESP32 will NOT appear in iPhone Settings > Bluetooth. You must use a BLE scanner app.

3. **Scan for Devices**
   - Open your chosen BLE scanner app
   - Start scanning for nearby devices
   - Look for "ESP32_IRK_FINDER" in the device list

4. **Connect and Pair**
   - Tap on "ESP32_IRK_FINDER" to connect
   - When prompted for pairing, tap "Pair"
   - Enter passkey: **123456**
   - Confirm the pairing

5. **View Retrieved IRK**
   - Return to the web interface
   - The IRK will be displayed in multiple formats
   - Use the copy buttons to copy the desired format

### Web Interface Features

The web interface displays the IRK in four formats:

1. **Standard Hex** - Direct hexadecimal representation
2. **ESPresense Format** - Reversed byte order for ESPresense
3. **Base64** - For Home Assistant Private BLE integration
4. **Hex Array** - Comma-separated hex values for programming

Each format has a dedicated copy button for easy clipboard access.

### Resetting for a New Device

When you want to retrieve the IRK from a different iPhone:

1. Click the **Reset IRK** button that appears after an IRK is retrieved
2. Confirm the reset action when prompted
3. The current IRK will be cleared and all paired devices removed
4. Follow the pairing steps again with the new iPhone

The reset function:
- Clears the stored IRK immediately
- Removes all Bluetooth bonded devices
- Does not require restarting the ESP32
- Allows you to pair with a new device right away

---

## IRK Format Explanations

### Standard Hex Format
```
112233445566778899AABBCCDDEEFF00
```
- Direct representation of the 16-byte IRK
- Most commonly used format
- Each pair represents one byte

### ESPresense Format (Reversed)
```
00FFEEDDCCBBAA998877665544332211
```
- Byte order reversed for ESPresense compatibility
- Required by some tracking systems
- Same data, different endianness

### Base64 Format
```
ESIzRFVmd4iZqrvM3e7/AA==
```
- Base64 encoded version of the IRK
- Used by Home Assistant Private BLE Device integration
- Compact representation

### Hex Array Format
```
0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00
```
- C-style array format
- Useful for programming and debugging
- Easy to paste into code

---

## Troubleshooting

### Common Issues and Solutions

#### Can't Connect to WiFi
- **Solution:** Use the web configuration portal in AP mode
- Clear saved credentials via web interface and reconfigure
- Check WiFi password is correct
- Ensure router is 2.4GHz compatible

#### iPhone Doesn't Show ESP32
- **Issue:** Device not visible in iPhone Settings > Bluetooth
- **Solution:** This is expected behavior. Use a BLE scanner app instead
- The ESP32 only appears in BLE scanner apps, not in system Bluetooth settings

#### IRK Not Retrieved
- **Possible Causes:**
  - Pairing not completed
  - Wrong passkey entered
  - Connection dropped during pairing
- **Solutions:**
  - Ensure you accept the pairing request
  - Confirm passkey is exactly: 123456
  - Try disconnecting and reconnecting in the BLE app
  - Power cycle the ESP32 and try again

#### Web Interface Not Accessible
- **Check:**
  - Correct IP address (check serial output)
  - Device is on same network
  - Firewall not blocking port 80
- **Alternative:** Use AP mode (192.168.4.1)

#### "Encryption Insufficient" Error
- **Solution:** This issue has been resolved in the latest firmware
- Ensure you're using the latest firmware version
- Try clearing app cache or using different BLE scanner app

#### WiFi Credentials Lost
- **Solution:**
  - Connect via AP mode
  - Navigate to WiFi configuration page
  - Clear credentials and reconfigure

#### Copy Buttons Not Working
- **Browser Compatibility:**
  - Works best in Chrome/Edge
  - May have issues in Safari
  - Try different browser if problems persist

### Serial Monitor Debugging

Connect to serial monitor at 115200 baud to see:
- Device initialization status
- WiFi connection status and IP address
- BLE pairing events
- IRK retrieval confirmation
- Error messages and warnings

### LED Indicators

The built-in LED (GPIO 2) shows status:
- **Blinking:** Device initializing
- **Solid:** Ready for connections
- **Fast blink:** Active BLE connection

---

## Next Steps

- [Understand the API endpoints](api.md) for integration
- [Customize the configuration](configuration.md) for your needs
- [Learn technical details](technical.md) about the implementation

---

[← Back to README](../README.md) | [API Documentation →](api.md)