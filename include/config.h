#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
// These can be overridden by values from .env file
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// Alternative: Use AP mode instead of connecting to WiFi
#ifndef USE_AP_MODE
#define USE_AP_MODE 0
#endif

#ifndef AP_SSID
#define AP_SSID "ESP32-IRK-FINDER"
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD "12345678"
#endif

// BLE Configuration
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "ESP32_IRK_FINDER"
#endif

#ifndef BLE_PASSKEY
#define BLE_PASSKEY 123456
#endif

// Web Server Configuration
#ifndef WEB_SERVER_PORT
#define WEB_SERVER_PORT 80
#endif

// LED Configuration (built-in LED on most ESP32 boards)
#ifndef LED_PIN
#define LED_PIN 2
#endif

#endif