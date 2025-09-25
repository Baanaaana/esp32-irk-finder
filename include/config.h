#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Alternative: Use AP mode instead of connecting to WiFi
#define USE_AP_MODE false
#define AP_SSID "ESP32-IRK-FINDER"
#define AP_PASSWORD "12345678"

// BLE Configuration
#define BLE_DEVICE_NAME "ESP32_IRK_FINDER"
#define BLE_PASSKEY 123456

// Web Server Configuration
#define WEB_SERVER_PORT 80

// LED Configuration (built-in LED on most ESP32 boards)
#define LED_PIN 2

#endif