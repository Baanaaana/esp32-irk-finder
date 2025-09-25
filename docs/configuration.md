# Configuration Guide

[← Back to README](../README.md)

## Table of Contents
- [Configuration File](#configuration-file)
- [Default Settings](#default-settings)
- [Customization Options](#customization-options)
- [Build Flags](#build-flags)
- [Partition Scheme](#partition-scheme)

---

## Configuration File

The main configuration is stored in `include/config.h`. Create this file if it doesn't exist:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// Access Point Configuration
#define AP_SSID "ESP32-IRK-FINDER"
#define AP_PASSWORD "12345678"

// Bluetooth Configuration
#define BLE_DEVICE_NAME "ESP32_IRK_FINDER"
#define BLE_PASSKEY 123456

// Hardware Configuration
#define LED_PIN 2

// Web Server Configuration
#define WEB_SERVER_PORT 80

// mDNS Hostname (access as esp32-irk-finder.local)
#define MDNS_HOSTNAME "esp32-irk-finder"

#endif
```

---

## Default Settings

### Access Point Mode
- **SSID:** `ESP32-IRK-FINDER`
- **Password:** `12345678`
- **IP Address:** `192.168.4.1`
- **Channel:** 1

### Bluetooth Low Energy
- **Device Name:** `ESP32_IRK_FINDER`
- **Passkey:** `123456`
- **IO Capability:** ESP_IO_CAP_NONE
- **Security Mode:** Authenticated pairing with encryption

### Web Server
- **Port:** 80
- **Auto-refresh:** 2 seconds
- **Max clients:** 4

### Serial Communication
- **Baud Rate:** 115200
- **Data Bits:** 8
- **Stop Bits:** 1
- **Parity:** None

---

## Customization Options

### Change Access Point Credentials

Edit `config.h`:
```cpp
#define AP_SSID "MyCustomAP"
#define AP_PASSWORD "MySecurePassword123"
```

**Requirements:**
- SSID: 1-32 characters
- Password: 8-63 characters (or empty for open network)

### Change BLE Device Name

Edit `config.h`:
```cpp
#define BLE_DEVICE_NAME "MY_IRK_DEVICE"
```

**Requirements:**
- Maximum 29 characters
- Alphanumeric and underscores only
- Will be visible in BLE scanner apps

### Change BLE Passkey

Edit `config.h`:
```cpp
#define BLE_PASSKEY 654321
```

**Requirements:**
- 6-digit number (000000-999999)
- Used during pairing process
- Update documentation if changed

### Change LED Pin

Edit `config.h`:
```cpp
#define LED_PIN 13  // Use GPIO 13 instead of 2
```

**Common LED pins:**
- ESP32: GPIO 2 (built-in)
- ESP32-S3: GPIO 48 (RGB LED)
- ESP32-C3: GPIO 8 (built-in)

### Change Web Server Port

Edit `config.h`:
```cpp
#define WEB_SERVER_PORT 8080  // Use port 8080 instead of 80
```

**Note:** Remember to include port in URL if not 80

---

## Build Flags

### PlatformIO Build Flags

In `platformio.ini`:

```ini
build_flags =
    -DCORE_DEBUG_LEVEL=3          ; Debug level (0-5)
    -DCONFIG_BT_ENABLED           ; Enable Bluetooth
    -DCONFIG_BLUEDROID_ENABLED    ; Use Bluedroid stack
    -DCONFIG_CLASSIC_BT_ENABLED   ; Enable Classic BT (ESP32 only)
    -DCONFIG_BT_SPP_ENABLED       ; Enable SPP (ESP32 only)
```

### Debug Levels
- `0` - None
- `1` - Error
- `2` - Warn
- `3` - Info (default)
- `4` - Debug
- `5` - Verbose

### Disable Debug Output

```ini
build_flags =
    -DCORE_DEBUG_LEVEL=0
```

### Custom Defines

Add your own build flags:
```ini
build_flags =
    ${env.build_flags}
    -DCUSTOM_FEATURE=1
    -DMAX_CONNECTIONS=10
```

---

## Partition Scheme

### Current Scheme: huge_app.csv

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x300000
spiffs,   data, spiffs,  0x310000,0xF0000
```

**Sizes:**
- Application: 3MB
- SPIFFS: 960KB
- NVS: 20KB

### Alternative Schemes

**For smaller applications:**
```ini
board_build.partitions = default.csv
```

**For OTA updates:**
```ini
board_build.partitions = min_spiffs.csv
```

**Custom partition table:**
1. Create `partitions.csv` in project root
2. Update `platformio.ini`:
```ini
board_build.partitions = partitions.csv
```

---

## Platform-Specific Configuration

### ESP32 Original
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.partitions = huge_app.csv

build_flags =
    -DCONFIG_CLASSIC_BT_ENABLED
    -DCONFIG_BT_SPP_ENABLED
```

### ESP32-S3
```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
board_build.partitions = huge_app.csv

build_flags =
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
```

### ESP32-C3
```ini
[env:esp32c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
board_build.partitions = huge_app.csv

build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=1
```

---

## Advanced Configuration

### Memory Management

**Stack size adjustment:**
```cpp
#define CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY true
#define CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_INTERNAL 1
```

### WiFi Power Management

```cpp
// In setup()
WiFi.setSleep(false);  // Disable WiFi sleep
WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Set TX power
```

### BLE Advertisement Settings

```cpp
// In BLE initialization
esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,  // 20ms
    .adv_int_max = 0x40,  // 40ms
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};
```

### DNS Settings

```cpp
// Custom DNS server IP
IPAddress DNS_IP(192, 168, 4, 1);
```

### Timeout Settings

```cpp
#define WIFI_CONNECT_TIMEOUT 10000  // 10 seconds
#define BLE_SCAN_DURATION 30         // 30 seconds
#define WEB_REQUEST_TIMEOUT 5000     // 5 seconds
```

---

## Persistent Storage

### WiFi Credentials
- Stored using Preferences library
- Namespace: `wifi`
- Keys: `ssid`, `password`

### Clear stored data:
```cpp
preferences.begin("wifi", false);
preferences.clear();
preferences.end();
```

### Store custom data:
```cpp
preferences.begin("custom", false);
preferences.putString("key", "value");
preferences.end();
```

---

## Security Considerations

1. **Change default passwords** in production
2. **Use strong AP password** (WPA2)
3. **Change default BLE passkey**
4. **Disable debug output** in production
5. **Consider adding authentication** to web interface
6. **Use HTTPS** if possible (requires certificates)

---

[← Back to README](../README.md) | [Technical Details →](technical.md)