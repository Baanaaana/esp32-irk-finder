# Technical Documentation

[← Back to README](../README.md)

## Table of Contents
- [Architecture Overview](#architecture-overview)
- [BLE Implementation](#ble-implementation)
- [IRK Retrieval Process](#irk-retrieval-process)
- [Web Server Implementation](#web-server-implementation)
- [WiFi Management](#wifi-management)
- [Memory and Performance](#memory-and-performance)

---

## Architecture Overview

### Component Stack

```
┌─────────────────────────────┐
│      Web Interface          │
├─────────────────────────────┤
│    AsyncWebServer (HTTP)    │
├─────────────────────────────┤
│      Application Logic      │
├──────────┬──────────────────┤
│   BLE    │   WiFi Manager   │
│ Bluedroid│   (ESP WiFi)     │
├──────────┴──────────────────┤
│        ESP32 Core           │
│      (Arduino Framework)    │
└─────────────────────────────┘
```

### Main Components

1. **BLE Stack (Bluedroid)**
   - Handles Bluetooth Low Energy communication
   - Manages pairing and bonding
   - Retrieves IRK during key exchange

2. **AsyncWebServer**
   - Non-blocking HTTP server
   - Handles multiple clients simultaneously
   - Serves static HTML and API endpoints

3. **WiFi Manager**
   - Manages AP and Station modes
   - Handles credential storage
   - Implements captive portal

4. **Preferences Library**
   - Persistent storage for WiFi credentials
   - NVS (Non-Volatile Storage) backend

---

## BLE Implementation

### Bluedroid Stack Choice

The project uses ESP-IDF's Bluedroid stack instead of NimBLE for iOS compatibility:

**Advantages:**
- Better iOS device compatibility
- Direct IRK access during pairing
- Stable secure pairing implementation
- Native ESP32 support

**Key Components:**
- GAP (Generic Access Profile) for device discovery
- GATT (Generic Attribute Profile) for services
- SMP (Security Manager Protocol) for pairing

### GATT Server Structure

```cpp
// Service and Characteristic UUIDs
#define HEART_RATE_SERVICE_UUID       0x180D
#define HEART_RATE_MEASUREMENT_UUID   0x2A37
#define BODY_SENSOR_LOCATION_UUID     0x2A38

// GATT Database
gatt_db[] = {
    // Primary Service
    {ESP_GATT_AUTO_RSP,
     {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid},
     ESP_GATT_PERM_READ,
     sizeof(heart_rate_service_uuid),
     (uint8_t *)&heart_rate_service_uuid},

    // Characteristics requiring encryption
    {ESP_GATT_AUTO_RSP,
     {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid},
     ESP_GATT_PERM_READ | ESP_GATT_PERM_READ_ENCRYPTED,
     ...}
}
```

### Security Configuration

```cpp
// IO Capability - No input/output
esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;

// Authentication requirements
esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_BOND;

// Security parameters
esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req);
esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap);
esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey);
```

### Random Address Generation

For iOS compatibility, the device uses a random static address:

```cpp
void generateRandomAddress(esp_bd_addr_t addr) {
    esp_fill_random(addr, 6);
    addr[0] = (addr[0] & 0x3F) | 0xC0;  // Random static address
}

// Set random address
esp_bd_addr_t rand_addr;
generateRandomAddress(rand_addr);
esp_ble_gap_set_rand_addr(rand_addr);
```

---

## IRK Retrieval Process

### Key Exchange Flow

```
iPhone                    ESP32
  │                         │
  ├──── BLE Scan ──────────>│
  │<─── Advertisement ──────│
  │                         │
  ├──── Connection Req ────>│
  │<─── Connection Rsp ─────│
  │                         │
  ├──── Pairing Req ───────>│
  │<─── Pairing Rsp ────────│
  │                         │
  ├──── Key Exchange ──────>│
  │     (IRK included)      │
  │<─── Key Exchange ───────│
  │                         │
  └─── Pairing Complete ────┘
```

### IRK Capture Methods

The firmware uses two methods to capture the IRK:

#### Method 1: GAP Event Handler
```cpp
case ESP_GAP_BLE_KEY_EVT:
    if (param->ble_security.ble_key.key_type == ESP_LE_KEY_PID) {
        // Direct IRK capture during key exchange
        uint8_t* irk_bytes = param->ble_security.ble_key.p_key_value.pid_key.irk;
        memcpy(retrieved_irk, irk_bytes, 16);
        irk_retrieved = true;
    }
    break;
```

#### Method 2: Bond Information
```cpp
case ESP_GAP_BLE_AUTH_CMPL_EVT:
    if (param->ble_security.auth_cmpl.success) {
        // Retrieve IRK from bond information
        esp_ble_get_bond_device_list(&dev_num, dev_list);
        for (int i = 0; i < dev_num; i++) {
            memcpy(retrieved_irk, dev_list[i].bond_key.pid_key.irk, 16);
            irk_retrieved = true;
        }
    }
    break;
```

### IRK Format Conversions

```cpp
// Standard Hex
for (int i = 0; i < 16; i++) {
    sprintf(&irk_hex[i * 2], "%02X", retrieved_irk[i]);
}

// Reversed (ESPresense)
for (int i = 0; i < 16; i++) {
    sprintf(&irk_reversed[i * 2], "%02X", retrieved_irk[15 - i]);
}

// Base64
size_t outputLength;
mbedtls_base64_encode((unsigned char*)irk_base64, sizeof(irk_base64),
                      &outputLength, retrieved_irk, 16);

// Hex Array
for (int i = 0; i < 16; i++) {
    offset += sprintf(irk_array + offset, "0x%02X", retrieved_irk[i]);
    if (i < 15) offset += sprintf(irk_array + offset, ",");
}
```

---

## Web Server Implementation

### AsyncWebServer Architecture

Non-blocking implementation using ESPAsyncWebServer:

```cpp
AsyncWebServer server(80);
AsyncEventSource events("/events");

void setupWebServer() {
    // Static routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleWifiConfig);

    // API routes
    server.on("/api/status", HTTP_GET, handleApiStatus);
    server.on("/api/wifi/save", HTTP_POST, handleWifiSave);

    // 404 handler
    server.onNotFound(handleNotFound);

    server.begin();
}
```

### HTML Generation

Dynamic HTML generation with embedded CSS:

```cpp
String html = "<!DOCTYPE html><html><head>";
html += "<style>";
html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', "
        "Roboto, sans-serif; }";
html += ".container { max-width: 800px; margin: 0 auto; padding: 20px; }";
html += "</style>";
html += "</head><body>";
// ... dynamic content
html += "</body></html>";
```

### JSON API Responses

```cpp
void handleApiStatus(AsyncWebServerRequest *request) {
    StaticJsonDocument<512> doc;
    doc["irk"] = irk_hex;
    doc["irkReversed"] = irk_reversed;
    doc["irkBase64"] = irk_base64;
    doc["irkArray"] = irk_array;
    doc["mac"] = mac_address;
    doc["irkRetrieved"] = irk_retrieved;
    doc["isAPMode"] = WiFi.getMode() == WIFI_AP;
    doc["ipAddress"] = WiFi.localIP().toString();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}
```

---

## WiFi Management

### Dual Mode Operation

The device supports both AP and Station modes:

```cpp
void setupWiFi() {
    preferences.begin("wifi", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    if (ssid.length() > 0) {
        // Try Station mode
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            // Fall back to AP mode
            startAPMode();
        }
    } else {
        // No credentials, start AP mode
        startAPMode();
    }
}
```

### Captive Portal Implementation

```cpp
DNSServer dnsServer;

void startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    // DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());

    // Redirect all requests to config page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->host() != WiFi.softAPIP().toString()) {
            request->redirect("http://" + WiFi.softAPIP().toString() + "/wifi");
        } else {
            handleRoot(request);
        }
    });
}
```

### Credential Persistence

```cpp
void saveWiFiCredentials(String ssid, String password) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
}

void clearWiFiCredentials() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
}
```

---

## Memory and Performance

### Memory Usage

**Flash Usage (ESP32):**
- Application: ~1.2MB
- Bootloader: 25KB
- Partition table: 3KB

**RAM Usage:**
- Static: ~45KB
- Dynamic: ~30KB
- Stack: 8KB per task

### Optimization Techniques

1. **String Optimization**
```cpp
// Use PROGMEM for static strings
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
...
)rawliteral";
```

2. **Buffer Reuse**
```cpp
// Reuse buffers for different operations
char buffer[256];
// Use for IRK formatting
// Reuse for JSON responses
```

3. **Async Operations**
- Non-blocking web server
- Event-driven BLE handling
- Cooperative multitasking

### Performance Metrics

- **Boot time:** ~2 seconds
- **WiFi connection:** 3-5 seconds
- **BLE pairing:** 2-3 seconds
- **IRK retrieval:** < 1 second after pairing
- **Web response time:** < 50ms
- **API response time:** < 20ms

### Task Priority

```cpp
// Core 0: WiFi and networking
// Core 1: BLE and application logic

xTaskCreatePinnedToCore(
    wifiTask,      // Task function
    "WiFi",        // Name
    4096,          // Stack size
    NULL,          // Parameters
    1,             // Priority
    &wifiTaskHandle,
    0              // Core 0
);
```

---

## Debugging

### Serial Output Levels

```cpp
#define LOG_ERROR(x)   Serial.print("[ERROR] "); Serial.println(x)
#define LOG_WARNING(x) Serial.print("[WARN] "); Serial.println(x)
#define LOG_INFO(x)    Serial.print("[INFO] "); Serial.println(x)
#define LOG_DEBUG(x)   if (DEBUG) { Serial.print("[DEBUG] "); Serial.println(x) }
```

### Common Debug Points

1. **BLE Events**
```cpp
Serial.printf("GAP Event: %d\n", event);
Serial.printf("GATT Event: %d\n", event);
```

2. **IRK Capture**
```cpp
Serial.printf("IRK Retrieved: ");
for (int i = 0; i < 16; i++) {
    Serial.printf("%02X", retrieved_irk[i]);
}
Serial.println();
```

3. **WiFi Status**
```cpp
Serial.printf("WiFi Status: %d\n", WiFi.status());
Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
```

---

[← Back to README](../README.md) | [Development Guide →](development.md)