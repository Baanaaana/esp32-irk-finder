# API Documentation

[← Back to README](../README.md)

## Table of Contents
- [Overview](#overview)
- [IRK Finder Endpoints](#irk-finder-endpoints)
- [WiFi Configuration Endpoints](#wifi-configuration-endpoints)
- [Response Formats](#response-formats)
- [Integration Examples](#integration-examples)

---

## Overview

The ESP32 IRK Finder provides a RESTful API for retrieving IRK data and managing WiFi configuration. All endpoints are accessible via HTTP on port 80.

### Base URLs
- **When connected to WiFi:** `http://esp32-irk-finder.local/` or `http://<device-ip>/`
- **In AP mode:** `http://192.168.4.1/` or `http://esp32-irk-finder.local/`

---

## IRK Finder Endpoints

### GET /
**Description:** Main web interface for IRK display

**Response:** HTML page with IRK display and copy buttons

**Usage:**
```
http://esp32-irk-finder.local/
```

---

### GET /api/status
**Description:** Get current device status and IRK data in JSON format

**Response:**
```json
{
  "irk": "112233445566778899aabbccddeeff00",
  "irkReversed": "00ffeeddccbbaa998877665544332211",
  "irkBase64": "ESIzRFVmd4iZqrvM3e7/AA==",
  "irkArray": "0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,0x00",
  "mac": "AA:BB:CC:DD:EE:FF",
  "irkRetrieved": true,
  "isAPMode": false,
  "ipAddress": "192.168.1.100"
}
```

**Fields:**
- `irk` - Standard hex format (32 characters)
- `irkReversed` - Byte-reversed for ESPresense
- `irkBase64` - Base64 encoded
- `irkArray` - C-style hex array
- `mac` - MAC address of connected device
- `irkRetrieved` - Boolean indicating if IRK was successfully retrieved
- `isAPMode` - Boolean indicating if device is in AP mode
- `ipAddress` - Current IP address of the device

**Example Usage:**
```bash
curl http://esp32-irk-finder.local/api/status
```

---

## WiFi Configuration Endpoints

### GET /wifi
**Description:** WiFi configuration web interface

**Response:** HTML page with WiFi configuration form

---

### GET /api/wifi/status
**Description:** Get current WiFi connection status

**Response:**
```json
{
  "connected": true,
  "ssid": "MyNetwork",
  "ip": "192.168.1.100",
  "rssi": -65,
  "isAPMode": false
}
```

**Fields:**
- `connected` - WiFi connection status
- `ssid` - Connected network name (empty if not connected)
- `ip` - Current IP address
- `rssi` - Signal strength in dBm
- `isAPMode` - Whether device is in AP mode

---

### GET /api/wifi/scan
**Description:** Scan for available WiFi networks

**Note:** This endpoint has been deprecated and removed in the latest version.

---

### POST /api/wifi/save
**Description:** Save WiFi credentials and connect

**Request Body:**
```json
{
  "ssid": "MyNetwork",
  "password": "MyPassword"
}
```

**Response:**
```json
{
  "success": true,
  "message": "WiFi credentials saved. Restarting..."
}
```

**Notes:**
- Device will restart after saving credentials
- Credentials are stored persistently
- Connection attempt happens after restart

**Example Usage:**
```bash
curl -X POST http://192.168.4.1/api/wifi/save \
  -H "Content-Type: application/json" \
  -d '{"ssid":"MyNetwork","password":"MyPassword"}'
```

---

### POST /api/wifi/clear
**Description:** Clear saved WiFi credentials

**Response:**
```json
{
  "success": true,
  "message": "WiFi credentials cleared. Restarting..."
}
```

**Notes:**
- Device will restart in AP mode after clearing
- All saved credentials will be removed

**Example Usage:**
```bash
curl -X POST http://192.168.1.100/api/wifi/clear
```

---

### POST /api/reset
**Description:** Reset IRK and clear all paired devices

**Response:**
```json
{
  "success": true
}
```

**Notes:**
- Clears the currently stored IRK
- Removes all Bluetooth bonded devices
- Allows pairing with a new iPhone
- Does not restart the device
- Web interface will reflect the reset immediately

**Example Usage:**
```bash
curl -X POST http://192.168.1.100/api/reset
```

---

## Response Formats

### Success Response
```json
{
  "success": true,
  "message": "Operation completed successfully",
  "data": { ... }
}
```

### Error Response
```json
{
  "success": false,
  "error": "Error description",
  "code": "ERROR_CODE"
}
```

### Common HTTP Status Codes
- `200 OK` - Successful request
- `400 Bad Request` - Invalid request parameters
- `404 Not Found` - Endpoint not found
- `500 Internal Server Error` - Server error

---

## Integration Examples

### Home Assistant Integration

**Using RESTful sensor:**
```yaml
sensor:
  - platform: rest
    name: "iPhone IRK"
    resource: http://192.168.1.100/api/status
    value_template: "{{ value_json.irkBase64 }}"
    json_attributes:
      - irk
      - irkReversed
      - mac
      - irkRetrieved
```

**Using Private BLE Device:**
```yaml
ble_monitor:
  devices:
    - mac: "Random"
      irk: "ESIzRFVmd4iZqrvM3e7/AA=="  # Use irkBase64 from API
      name: "iPhone"
```

### ESPresense Configuration

Use the `irkReversed` field from the API:

```yaml
irk:
  "00ffeeddccbbaa998877665544332211": "iPhone"
```

### Python Integration

```python
import requests
import json

# Get IRK data
response = requests.get('http://192.168.1.100/api/status')
data = response.json()

if data['irkRetrieved']:
    print(f"IRK: {data['irk']}")
    print(f"Base64: {data['irkBase64']}")
    print(f"ESPresense: {data['irkReversed']}")
```

### JavaScript/Node.js Integration

```javascript
fetch('http://192.168.1.100/api/status')
  .then(response => response.json())
  .then(data => {
    if (data.irkRetrieved) {
      console.log('IRK:', data.irk);
      console.log('Base64:', data.irkBase64);
      console.log('ESPresense:', data.irkReversed);
    }
  });
```

### Shell Script Integration

```bash
#!/bin/bash

# Get IRK in standard format
IRK=$(curl -s http://192.168.1.100/api/status | jq -r '.irk')
echo "IRK: $IRK"

# Get IRK in Base64 format
IRK_BASE64=$(curl -s http://192.168.1.100/api/status | jq -r '.irkBase64')
echo "Base64: $IRK_BASE64"
```

---

## Rate Limiting

- No rate limiting is implemented
- API calls are processed synchronously
- Recommended polling interval: 2+ seconds

---

## Security Considerations

- API endpoints are not authenticated
- Ensure device is on trusted network
- IRK is sensitive information - secure your network
- Consider implementing firewall rules to restrict access

---

[← Back to README](../README.md) | [Configuration →](configuration.md)