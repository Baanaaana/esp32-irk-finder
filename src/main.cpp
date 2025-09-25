/*
 * ESP32 IRK Finder - Based on ESP-IDF Bluedroid example
 * Retrieves iPhone IRK through BLE pairing
 */

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp32-hal.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "config.h"

// Web server
AsyncWebServer server(WEB_SERVER_PORT);

// DNS server for captive portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Preferences for storing WiFi credentials
Preferences preferences;

// mDNS hostname
const char* mdnsHostname = "esp32-irk-finder";

// WiFi configuration
String stored_ssid = "";
String stored_password = "";
bool isAPMode = false;

// Global IRK storage
String currentIRK = "No IRK retrieved yet";
String currentIRKBase64 = "";
String currentIRKReversed = "";
String currentIRKArray = "";
String connectedDeviceMAC = "None";
bool irkRetrieved = false;

// Helper function to encode bytes to Base64
String base64Encode(const uint8_t* data, size_t length) {
    const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result;

    for (size_t i = 0; i < length; i += 3) {
        uint32_t value = data[i] << 16;
        if (i + 1 < length) value |= data[i + 1] << 8;
        if (i + 2 < length) value |= data[i + 2];

        result += table[(value >> 18) & 0x3F];
        result += table[(value >> 12) & 0x3F];
        result += (i + 1 < length) ? table[(value >> 6) & 0x3F] : '=';
        result += (i + 2 < length) ? table[value & 0x3F] : '=';
    }

    return result;
}

// Helper function to reverse IRK bytes for ESPresense
String reverseIRK(const uint8_t* irk) {
    char reversed[33];
    for (int i = 0; i < 16; i++) {
        sprintf(&reversed[i * 2], "%02x", irk[15 - i]);
    }
    reversed[32] = '\0';
    return String(reversed);
}

// Helper function to format IRK as hex array
String formatIRKArray(const uint8_t* irk) {
    String result = "";
    for (int i = 0; i < 16; i++) {
        if (i > 0) result += ",";
        char hex[6];
        sprintf(hex, "0x%02x", irk[i]);
        result += hex;
    }
    return result;
}

// BLE Configuration
#define GATTS_TABLE_TAG "ESP32_IRK"
#define HEART_PROFILE_NUM                         1
#define HEART_PROFILE_APP_IDX                     0
#define ESP_HEART_RATE_APP_ID                     0x55
#define EXAMPLE_DEVICE_NAME                       "ESP32_IRK_FINDER"
#define HEART_RATE_SVC_INST_ID                    0

#define ADV_CONFIG_FLAG                           (1 << 0)
#define SCAN_RSP_CONFIG_FLAG                      (1 << 1)

static uint8_t adv_config_done = 0;

// Attributes State Machine
enum {
    HRS_IDX_SVC,
    HRS_IDX_HR_MEAS_CHAR,
    HRS_IDX_HR_MEAS_VAL,
    HRS_IDX_HR_MEAS_NTF_CFG,
    HRS_IDX_BOBY_SENSOR_LOC_CHAR,
    HRS_IDX_BOBY_SENSOR_LOC_VAL,
    HRS_IDX_HR_CTNL_PT_CHAR,
    HRS_IDX_HR_CTNL_PT_VAL,
    HRS_IDX_NB,
};

static uint16_t heart_rate_handle_table[HRS_IDX_NB];
static uint8_t test_manufacturer[3] = {'E', 'S', 'P'};

static uint8_t sec_service_uuid[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0x18, 0x0D, 0x00, 0x00,
};

// Advertising configuration
esp_ble_adv_data_t heart_rate_adv_config = {};
esp_ble_adv_data_t heart_rate_scan_rsp_config = {};
esp_ble_adv_params_t heart_rate_adv_params = {};

// Random address for BLE (required for iOS compatibility)
static uint8_t rand_addr[6] = {0xC0, 0x01, 0x02, 0x03, 0x04, 0x05};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static struct gatts_profile_inst heart_rate_profile_tab[HEART_PROFILE_NUM] = {
    [HEART_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

// GATT Database
static const uint16_t GATTS_SERVICE_UUID_HEART_RATE = 0x180D;
static const uint16_t GATTS_CHAR_UUID_HEART_RATE_MEASUREMENT = 0x2A37;
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;

static const uint16_t heart_rate_meas_uuid = ESP_GATT_HEART_RATE_MEAS;
static const uint8_t heart_measurement_ccc[2] = {0x00, 0x00};
static const uint16_t body_sensor_location_uuid = ESP_GATT_BODY_SENSOR_LOCATION;
static const uint8_t body_sensor_loc_val[1] = {0x00};
static const uint16_t heart_rate_ctrl_point = ESP_GATT_HEART_RATE_CNTL_POINT;
static const uint8_t heart_ctrl_point[1] = {0x00};

// HTML page for web interface
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>ESP32 IRK Finder</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        :root {
            --background: #ffffff;
            --foreground: #0a0a0a;
            --card: #ffffff;
            --card-foreground: #0a0a0a;
            --popover: #ffffff;
            --popover-foreground: #0a0a0a;
            --primary: #171717;
            --primary-foreground: #fafafa;
            --secondary: #f5f5f5;
            --secondary-foreground: #171717;
            --muted: #f5f5f5;
            --muted-foreground: #737373;
            --accent: #f5f5f5;
            --accent-foreground: #171717;
            --destructive: #ef4444;
            --destructive-foreground: #fafafa;
            --border: #e5e5e5;
            --input: #e5e5e5;
            --ring: #171717;
            --radius: 0.5rem;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "Roboto", "Oxygen", "Ubuntu", "Cantarell", "Fira Sans", "Droid Sans", "Helvetica Neue", sans-serif;
            line-height: 1.5;
            color: var(--foreground);
            background: linear-gradient(to bottom, #fafafa, #f5f5f5);
            min-height: 100vh;
            padding: 1rem;
        }

        .container {
            max-width: 48rem;
            margin: 0 auto;
            padding: 2rem;
        }

        h1 {
            font-size: 2rem;
            font-weight: 700;
            text-align: center;
            margin-bottom: 2rem;
            background: linear-gradient(to right, #171717, #404040);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }

        .alert {
            padding: 0.75rem 1rem;
            border-radius: var(--radius);
            margin-bottom: 1.5rem;
            font-size: 0.875rem;
            font-weight: 500;
            text-align: center;
        }

        .alert-success {
            background-color: #dcfce7;
            color: #166534;
            border: 1px solid #bbf7d0;
        }

        .alert-warning {
            background-color: #fef3c7;
            color: #92400e;
            border: 1px solid #fde68a;
        }

        .card {
            background: var(--card);
            border-radius: var(--radius);
            border: 1px solid var(--border);
            box-shadow: 0 1px 3px 0 rgb(0 0 0 / 0.1), 0 1px 2px -1px rgb(0 0 0 / 0.1);
            margin-bottom: 1rem;
        }

        .card-content {
            padding: 1.5rem;
        }

        .label {
            font-size: 0.875rem;
            font-weight: 600;
            color: var(--foreground);
            margin-bottom: 0.25rem;
        }

        .description {
            font-size: 0.75rem;
            color: var(--muted-foreground);
            margin-bottom: 0.5rem;
        }

        .input-group {
            position: relative;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }

        .input {
            flex: 1;
            padding: 0.625rem 0.875rem;
            font-size: 0.875rem;
            font-family: 'SF Mono', 'Monaco', 'Inconsolata', 'Fira Code', monospace;
            background: var(--background);
            border: 1px solid var(--border);
            border-radius: calc(var(--radius) - 2px);
            color: var(--foreground);
            word-break: break-all;
            transition: border-color 0.2s;
        }

        .input:focus {
            outline: none;
            border-color: var(--ring);
            box-shadow: 0 0 0 3px rgb(23 23 23 / 0.05);
        }

        .btn {
            padding: 0.5rem 1rem;
            font-size: 0.875rem;
            font-weight: 500;
            border-radius: calc(var(--radius) - 2px);
            border: 1px solid transparent;
            cursor: pointer;
            transition: all 0.2s;
            display: inline-flex;
            align-items: center;
            justify-content: center;
        }

        .btn-primary {
            background: var(--primary);
            color: var(--primary-foreground);
            border-color: var(--primary);
        }

        .btn-primary:hover {
            background: #262626;
        }

        .btn-outline {
            background: var(--background);
            color: var(--foreground);
            border-color: var(--border);
        }

        .btn-outline:hover {
            background: var(--secondary);
        }

        .btn-black {
            background: var(--primary);
            color: var(--primary-foreground);
            border-color: var(--primary);
        }

        .btn-black:hover {
            background: #262626;
        }

        .btn-copy {
            padding: 0.375rem 0.75rem;
            font-size: 0.75rem;
            white-space: nowrap;
        }

        .btn-danger {
            background: var(--destructive);
            color: var(--destructive-foreground);
            border-color: var(--destructive);
        }

        .btn-danger:hover {
            background: #dc2626;
        }

        .reset-container {
            text-align: center;
            margin-top: 1.5rem;
        }

        .instructions {
            background: var(--muted);
            border-radius: var(--radius);
            padding: 1.5rem;
            margin-top: 1.5rem;
        }

        .instructions h3 {
            font-size: 1rem;
            font-weight: 600;
            margin-bottom: 1rem;
            color: var(--foreground);
        }

        .instructions ol {
            padding-left: 1.5rem;
            color: var(--muted-foreground);
        }

        .instructions li {
            margin-bottom: 0.5rem;
            font-size: 0.875rem;
        }

        .instructions code {
            background: var(--primary);
            color: var(--primary-foreground);
            padding: 0.125rem 0.375rem;
            border-radius: 0.25rem;
            font-weight: 600;
            font-size: 0.875rem;
        }

        .hidden {
            display: none;
        }

        @media (max-width: 640px) {
            .container {
                padding: 1rem;
            }

            h1 {
                font-size: 1.5rem;
            }

            .card-content {
                padding: 1rem;
            }
        }
    </style>
    <script>
        function copyToClipboard(elementId, btnElement) {
            const element = document.getElementById(elementId);
            const text = element.value || element.textContent;

            // Create a temporary textarea for copying
            const tempTextArea = document.createElement('textarea');
            tempTextArea.value = text;
            tempTextArea.style.position = 'fixed';
            tempTextArea.style.left = '-999999px';
            tempTextArea.style.top = '-999999px';
            document.body.appendChild(tempTextArea);
            tempTextArea.select();

            try {
                document.execCommand('copy');
                const originalText = btnElement.textContent;
                btnElement.textContent = 'Copied!';
                btnElement.style.background = '#22c55e';

                setTimeout(() => {
                    btnElement.textContent = originalText;
                    btnElement.style.background = '';
                }, 2000);
            } catch (err) {
                console.error('Failed to copy text: ', err);
            }

            document.body.removeChild(tempTextArea);
        }

        function refreshData() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('irk').value = data.irk || '';
                    document.getElementById('irk-reversed').value = data.irkReversed || '';
                    document.getElementById('irk-base64').value = data.irkBase64 || '';
                    document.getElementById('irk-array').value = data.irkArray || '';
                    document.getElementById('mac').value = data.mac || 'None';

                    const statusDiv = document.getElementById('status');
                    const formatsDiv = document.getElementById('irk-formats');
                    const macDiv = document.getElementById('mac-container');
                    const wifiConfigDiv = document.getElementById('wifi-config-link');
                    const resetDiv = document.getElementById('reset-container');

                    if (data.irkRetrieved) {
                        statusDiv.className = 'alert alert-success';
                        statusDiv.textContent = 'IRK Successfully Retrieved!';
                        formatsDiv.classList.remove('hidden');
                        macDiv.classList.remove('hidden');
                        resetDiv.classList.remove('hidden');
                    } else {
                        statusDiv.className = 'alert alert-warning';
                        statusDiv.textContent = 'Waiting for iPhone pairing...';
                        formatsDiv.classList.add('hidden');
                        macDiv.classList.add('hidden');
                        resetDiv.classList.add('hidden');
                    }

                    // Show WiFi config link if in AP mode
                    if (data.isAPMode && wifiConfigDiv) {
                        wifiConfigDiv.classList.remove('hidden');
                    }
                });
        }

        function resetIRK() {
            if (confirm('Are you sure you want to reset the IRK? This will clear the current IRK and remove all paired devices.')) {
                fetch('/api/reset', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            alert('IRK reset successfully. You can now pair a new device.');
                            location.reload();
                        } else {
                            alert('Failed to reset IRK: ' + (data.error || 'Unknown error'));
                        }
                    })
                    .catch(error => {
                        alert('Error resetting IRK: ' + error);
                    });
            }
        }

        setInterval(refreshData, 2000);
        window.onload = refreshData;
    </script>
</head>
<body>
    <div class="container">
        <h1>ESP32 IRK Finder</h1>

        <div id="status" class="alert alert-warning">Waiting for iPhone pairing...</div>

        <div id="mac-container" class="card hidden">
            <div class="card-content">
                <label class="label">Device MAC</label>
                <div class="input-group">
                    <input id="mac" class="input" readonly value="None">
                </div>
            </div>
        </div>

        <div id="irk-formats" class="hidden">
            <div class="card">
                <div class="card-content">
                    <label class="label">Standard IRK (Hex)</label>
                    <p class="description">Original format - use for debugging</p>
                    <div class="input-group">
                        <input id="irk" class="input" readonly>
                        <button class="btn btn-black btn-copy" onclick="copyToClipboard('irk', this)">Copy</button>
                    </div>
                </div>
            </div>

            <div class="card">
                <div class="card-content">
                    <label class="label">ESPresense Format (Reversed)</label>
                    <p class="description">Use with device_id: "irk:..." in ESPresense/Home Assistant</p>
                    <div class="input-group">
                        <input id="irk-reversed" class="input" readonly>
                        <button class="btn btn-black btn-copy" onclick="copyToClipboard('irk-reversed', this)">Copy</button>
                    </div>
                </div>
            </div>

            <div class="card">
                <div class="card-content">
                    <label class="label">Base64 Format</label>
                    <p class="description">For Home Assistant Private BLE Device integration</p>
                    <div class="input-group">
                        <input id="irk-base64" class="input" readonly>
                        <button class="btn btn-black btn-copy" onclick="copyToClipboard('irk-base64', this)">Copy</button>
                    </div>
                </div>
            </div>

            <div class="card">
                <div class="card-content">
                    <label class="label">Hex Array Format</label>
                    <p class="description">For custom implementations and programming</p>
                    <div class="input-group">
                        <input id="irk-array" class="input" readonly>
                        <button class="btn btn-black btn-copy" onclick="copyToClipboard('irk-array', this)">Copy</button>
                    </div>
                </div>
            </div>
        </div>

        <div id="reset-container" class="reset-container hidden">
            <button class="btn btn-danger" onclick="resetIRK()" style="padding: 0.625rem 2rem; width: auto;">Reset IRK</button>
            <p style="margin-top: 0.5rem; font-size: 0.875rem; color: var(--muted-foreground);">Clear IRK and remove all paired devices</p>
        </div>

        <div id="wifi-config-link" class="card hidden">
            <div class="card-content" style="text-align: center;">
                <p style="margin-bottom: 1rem;">You are in Access Point mode. Configure WiFi to connect to your network.</p>
                <a href="/wifi" class="btn btn-primary" style="display: inline-block; width: auto; padding: 0.625rem 2rem;">Configure WiFi</a>
            </div>
        </div>

        <div class="instructions">
            <h3>How to retrieve iPhone IRK</h3>
            <ol>
                <li>Download a BLE scanner app from the App Store:
                    <ul style="margin-top: 0.5rem; list-style-type: disc; padding-left: 1.5rem;">
                        <li><code>LightBlue</code></li>
                        <li><code>nRF Connect</code></li>
                        <li><code>Bluetooth Terminal</code></li>
                        <li><code>BLE Scanner</code></li>
                    </ul>
                </li>
                <li>Open the BLE scanner app and scan for devices</li>
                <li>Look for <code>ESP32_IRK_FINDER</code> in the device list</li>
                <li>Tap to connect</li>
                <li>When prompted for pairing, accept the request</li>
                <li>Enter passkey: <code>123456</code></li>
                <li>After successful pairing, the IRK will appear above in multiple formats</li>
            </ol>
        </div>
    </div>
</body>
</html>
)rawliteral";

// WiFi Configuration HTML page
const char wifi_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
    <title>WiFi Configuration - ESP32 IRK Finder</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        :root {
            --background: #ffffff; --foreground: #0a0a0a;
            --card: #ffffff; --card-foreground: #0a0a0a;
            --primary: #171717; --primary-foreground: #fafafa;
            --secondary: #f5f5f5; --secondary-foreground: #171717;
            --muted: #f5f5f5; --muted-foreground: #737373;
            --destructive: #ef4444; --destructive-foreground: #fafafa;
            --border: #e5e5e5; --input: #e5e5e5;
            --ring: #171717; --radius: 0.5rem;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "Roboto", "Oxygen", sans-serif;
            line-height: 1.5; color: var(--foreground);
            background: linear-gradient(to bottom, #fafafa, #f5f5f5);
            min-height: 100vh; padding: 1rem;
        }
        .container { max-width: 48rem; margin: 0 auto; padding: 2rem; }
        h1 {
            font-size: 2rem; font-weight: 700; text-align: center;
            margin-bottom: 2rem;
            background: linear-gradient(to right, #171717, #404040);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        .card {
            background: var(--card);
            border-radius: var(--radius);
            border: 1px solid var(--border);
            box-shadow: 0 1px 3px 0 rgb(0 0 0 / 0.1);
            margin-bottom: 1rem;
        }
        .card-content { padding: 1.5rem; }
        .form-group { margin-bottom: 1.5rem; }
        .label {
            font-size: 0.875rem; font-weight: 600;
            color: var(--foreground); margin-bottom: 0.5rem;
            display: block;
        }
        .input, .select {
            width: 100%; padding: 0.625rem 0.875rem;
            font-size: 0.875rem;
            background: var(--background);
            border: 1px solid var(--border);
            border-radius: calc(var(--radius) - 2px);
            color: var(--foreground);
            transition: border-color 0.2s;
        }
        .input:focus, .select:focus {
            outline: none;
            border-color: var(--ring);
            box-shadow: 0 0 0 3px rgb(23 23 23 / 0.05);
        }
        .btn {
            padding: 0.625rem 1.25rem;
            font-size: 0.875rem; font-weight: 500;
            border-radius: calc(var(--radius) - 2px);
            border: 1px solid transparent;
            cursor: pointer; transition: all 0.2s;
            width: 100%;
        }
        .btn-primary {
            background: var(--primary);
            color: var(--primary-foreground);
            border-color: var(--primary);
        }
        .btn-primary:hover { background: #262626; }
        .btn-secondary {
            background: var(--secondary);
            color: var(--secondary-foreground);
            border-color: var(--border);
        }
        .btn-secondary:hover { background: #e5e5e5; }
        .alert {
            padding: 0.75rem 1rem;
            border-radius: var(--radius);
            margin-bottom: 1.5rem;
            font-size: 0.875rem;
            font-weight: 500;
        }
        .alert-info {
            background-color: #dbeafe;
            color: #1e40af;
            border: 1px solid #93c5fd;
        }
        .networks-list {
            max-height: 300px;
            overflow-y: auto;
            border: 1px solid var(--border);
            border-radius: calc(var(--radius) - 2px);
            margin-bottom: 1rem;
        }
        .network-item {
            padding: 0.75rem 1rem;
            border-bottom: 1px solid var(--border);
            cursor: pointer;
            transition: background-color 0.2s;
        }
        .network-item:hover { background-color: var(--secondary); }
        .network-item:last-child { border-bottom: none; }
        .network-ssid { font-weight: 600; }
        .network-rssi {
            font-size: 0.75rem;
            color: var(--muted-foreground);
            margin-left: 0.5rem;
        }
        .loading { text-align: center; padding: 2rem; color: var(--muted-foreground); }
        .hidden { display: none; }
        .link {
            color: var(--primary);
            text-decoration: none;
            font-size: 0.875rem;
        }
        .link:hover { text-decoration: underline; }
        .mt-2 { margin-top: 1rem; }
    </style>
    <script>
        function saveWiFi() {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;

            if (!ssid) {
                alert('Please enter WiFi SSID');
                return;
            }

            fetch('/api/wifi/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid, password })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    alert('WiFi settings saved! ESP32 will restart and connect to the network.');
                    setTimeout(() => { window.location.href = '/'; }, 3000);
                } else {
                    alert('Failed to save WiFi settings');
                }
            });
        }

        function clearWiFi() {
            if (confirm('Clear saved WiFi credentials?')) {
                fetch('/api/wifi/clear', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        alert('WiFi credentials cleared');
                        location.reload();
                    });
            }
        }
    </script>
</head>
<body>
    <div class="container">
        <h1>WiFi Configuration</h1>

        <div class="alert alert-info">
            Connect ESP32 to your WiFi network
            <p style="font-size: 0.75rem; margin-top: 0.5rem;">Note: You can still retrieve IRKs in AP mode. <a href="/" style="color: #1e40af; text-decoration: underline;">Go to IRK Finder</a></p>
        </div>

        <div class="card">
            <div class="card-content">
                <div class="form-group">
                    <label class="label" for="ssid">WiFi SSID</label>
                    <input type="text" id="ssid" class="input" placeholder="Enter WiFi network name">
                </div>
                <div class="form-group">
                    <label class="label" for="password">WiFi Password</label>
                    <input type="password" id="password" class="input" placeholder="Enter WiFi password">
                </div>
                <button class="btn btn-primary" onclick="saveWiFi()">Save & Connect</button>
                <button class="btn btn-secondary mt-2" onclick="clearWiFi()">Clear Saved Credentials</button>
            </div>
        </div>

        <div style="text-align: center; margin-top: 2rem;">
            <a href="/" class="link">Back to IRK Finder</a>
        </div>
    </div>
</body>
</html>
)rawliteral";

// Full HRS Database Description
static const esp_gatts_attr_db_t heart_rate_gatt_db[HRS_IDX_NB] = {
    // Heart Rate Service Declaration
    [HRS_IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_HEART_RATE), (uint8_t *)&GATTS_SERVICE_UUID_HEART_RATE}
    },

    // Heart Rate Measurement Characteristic Declaration
    [HRS_IDX_HR_MEAS_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_notify}
    },

    // Heart Rate Measurement Characteristic Value
    [HRS_IDX_HR_MEAS_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&heart_rate_meas_uuid, ESP_GATT_PERM_READ,
         13, 0, NULL}
    },

    // Heart Rate Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HRS_IDX_HR_MEAS_NTF_CFG] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(uint16_t), sizeof(heart_measurement_ccc), (uint8_t *)heart_measurement_ccc}
    },

    // Body Sensor Location Characteristic Declaration
    [HRS_IDX_BOBY_SENSOR_LOC_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read}
    },

    // Body Sensor Location Characteristic Value
    [HRS_IDX_BOBY_SENSOR_LOC_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&body_sensor_location_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
         sizeof(uint8_t), sizeof(body_sensor_loc_val), (uint8_t *)body_sensor_loc_val}
    },

    // Heart Rate Control Point Characteristic Declaration
    [HRS_IDX_HR_CTNL_PT_CHAR] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_read_write}
    },

    // Heart Rate Control Point Characteristic Value
    [HRS_IDX_HR_CTNL_PT_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&heart_rate_ctrl_point, ESP_GATT_PERM_WRITE_ENCRYPTED | ESP_GATT_PERM_READ_ENCRYPTED,
         sizeof(uint8_t), sizeof(heart_ctrl_point), (uint8_t *)heart_ctrl_point}
    },
};

// Function to show bonded devices and extract IRK
static void show_bonded_devices(void) {
    int dev_num = esp_ble_get_bond_device_num();

    if(dev_num == 0) {
        ESP_LOGI(GATTS_TABLE_TAG, "No bonded devices");
        return;
    }

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    ESP_LOGI(GATTS_TABLE_TAG, "Bonded devices: %d", dev_num);

    for (int i = 0; i < dev_num; i++) {
        // Extract MAC address
        char mac_str[18];
        sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                dev_list[i].bd_addr[0], dev_list[i].bd_addr[1],
                dev_list[i].bd_addr[2], dev_list[i].bd_addr[3],
                dev_list[i].bd_addr[4], dev_list[i].bd_addr[5]);

        connectedDeviceMAC = String(mac_str);

        // Extract IRK
        char irk_str[33];
        uint8_t* irk_bytes = dev_list[i].bond_key.pid_key.irk;

        for(int j = 0; j < 16; j++) {
            sprintf(&irk_str[j * 2], "%02x", irk_bytes[j]);
        }
        irk_str[32] = '\0';

        currentIRK = String(irk_str);
        currentIRKBase64 = base64Encode(irk_bytes, 16);
        currentIRKReversed = reverseIRK(irk_bytes);
        currentIRKArray = formatIRKArray(irk_bytes);
        irkRetrieved = true;

        ESP_LOGI(GATTS_TABLE_TAG, "=================================");
        ESP_LOGI(GATTS_TABLE_TAG, "Device MAC: %s", mac_str);
        ESP_LOGI(GATTS_TABLE_TAG, "IRK (Hex): %s", irk_str);
        ESP_LOGI(GATTS_TABLE_TAG, "IRK (ESPresense): %s", currentIRKReversed.c_str());
        ESP_LOGI(GATTS_TABLE_TAG, "IRK (Base64): %s", currentIRKBase64.c_str());
        ESP_LOGI(GATTS_TABLE_TAG, "=================================");

        Serial.println("\n========================================");
        Serial.println("IRK SUCCESSFULLY RETRIEVED!");
        Serial.print("Device MAC: ");
        Serial.println(connectedDeviceMAC);
        Serial.println("\n--- IRK Formats ---");
        Serial.print("Standard Hex: ");
        Serial.println(currentIRK);
        Serial.print("ESPresense (reversed): ");
        Serial.println(currentIRKReversed);
        Serial.print("Base64 (HA Private BLE): ");
        Serial.println(currentIRKBase64);
        Serial.print("Hex Array: ");
        Serial.println(currentIRKArray);
        Serial.println("========================================\n");
    }

    free(dev_list);
}

// Remove all bonded devices
static void remove_all_bonded_devices(void) {
    int dev_num = esp_ble_get_bond_device_num();

    if(dev_num == 0) return;

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);

    for (int i = 0; i < dev_num; i++) {
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
    }

    free(dev_list);

    currentIRK = "No IRK retrieved yet";
    currentIRKBase64 = "";
    currentIRKReversed = "";
    currentIRKArray = "";
    connectedDeviceMAC = "None";
    irkRetrieved = false;
}

// GAP event handler
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGV(GATTS_TABLE_TAG, "GAP_EVT, event:%d", event);

    switch (event) {
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&heart_rate_adv_params);
            }
            break;

        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&heart_rate_adv_params);
            }
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising start failed");
            } else {
                ESP_LOGI(GATTS_TABLE_TAG, "Advertising started - Device name: ESP32_IRK_FINDER");
                Serial.println("BLE advertising started - look for 'ESP32_IRK_FINDER'");
            }
            break;

        case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "Passkey: %06d", param->ble_security.key_notif.passkey);
            Serial.printf("Passkey displayed: %06d\n", param->ble_security.key_notif.passkey);
            break;

        case ESP_GAP_BLE_NC_REQ_EVT:
            esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
            ESP_LOGI(GATTS_TABLE_TAG, "Numeric Comparison: %d", param->ble_security.key_notif.passkey);
            break;

        case ESP_GAP_BLE_SEC_REQ_EVT:
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;

        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            if(param->ble_security.auth_cmpl.success) {
                ESP_LOGI(GATTS_TABLE_TAG, "Authentication success!");
                Serial.println("Authentication completed successfully!");
                // Show bonded devices to extract IRK
                show_bonded_devices();
            } else {
                ESP_LOGI(GATTS_TABLE_TAG, "Authentication failed, reason: 0x%x",
                         param->ble_security.auth_cmpl.fail_reason);
                Serial.printf("Authentication failed, reason: 0x%x\n", param->ble_security.auth_cmpl.fail_reason);
            }
            break;

        case ESP_GAP_BLE_KEY_EVT:
            // Key exchange event - this is when we receive the IRK
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GAP_BLE_KEY_EVT, key type = %d", param->ble_security.ble_key.key_type);

            if (param->ble_security.ble_key.key_type == ESP_LE_KEY_PID) {
                // We received the IRK (Identity Resolving Key)
                ESP_LOGI(GATTS_TABLE_TAG, "Received IRK from peer device");

                // Extract and display the IRK immediately
                uint8_t* irk_bytes = param->ble_security.ble_key.p_key_value.pid_key.irk;
                char irk_str[33];
                for(int j = 0; j < 16; j++) {
                    sprintf(&irk_str[j * 2], "%02x", irk_bytes[j]);
                }
                irk_str[32] = '\0';

                currentIRK = String(irk_str);
                currentIRKBase64 = base64Encode(irk_bytes, 16);
                currentIRKReversed = reverseIRK(irk_bytes);
                currentIRKArray = formatIRKArray(irk_bytes);
                irkRetrieved = true;

                Serial.println("\n========================================");
                Serial.println("IRK RECEIVED VIA KEY EXCHANGE!");
                Serial.println("--- IRK Formats ---");
                Serial.print("Standard Hex: ");
                Serial.println(currentIRK);
                Serial.print("ESPresense (reversed): ");
                Serial.println(currentIRKReversed);
                Serial.print("Base64 (HA Private BLE): ");
                Serial.println(currentIRKBase64);
                Serial.print("Hex Array: ");
                Serial.println(currentIRKArray);
                Serial.println("========================================\n");
            }
            break;

        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT: {
            if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "config local privacy failed");
                break;
            }

            esp_err_t ret = esp_ble_gap_config_adv_data(&heart_rate_adv_config);
            if (ret) {
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed: %x", ret);
            } else {
                adv_config_done |= ADV_CONFIG_FLAG;
            }

            ret = esp_ble_gap_config_adv_data(&heart_rate_scan_rsp_config);
            if (ret) {
                ESP_LOGE(GATTS_TABLE_TAG, "config scan rsp data failed: %x", ret);
            } else {
                adv_config_done |= SCAN_RSP_CONFIG_FLAG;
            }
            break;
        }

        default:
            break;
    }
}

// GATTS profile event handler
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            esp_ble_gap_set_device_name(EXAMPLE_DEVICE_NAME);
            // Set random address for iOS compatibility
            esp_ble_gap_set_rand_addr(rand_addr);
            esp_ble_gap_config_local_privacy(true);
            esp_ble_gatts_create_attr_tab(heart_rate_gatt_db, gatts_if, HRS_IDX_NB, HEART_RATE_SVC_INST_ID);
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "Device connected");
            Serial.println("Device connected - starting security");

            // Start encryption with MITM protection immediately
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "Device disconnected");
            Serial.println("Device disconnected");
            esp_ble_gap_start_advertising(&heart_rate_adv_params);
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status == ESP_GATT_OK) {
                if(param->add_attr_tab.num_handle == HRS_IDX_NB) {
                    memcpy(heart_rate_handle_table, param->add_attr_tab.handles, sizeof(heart_rate_handle_table));
                    esp_ble_gatts_start_service(heart_rate_handle_table[HRS_IDX_SVC]);
                }
            }
            break;

        default:
            break;
    }
}

// Main GATTS event handler
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param) {
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            heart_rate_profile_tab[HEART_PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            return;
        }
    }

    for (int idx = 0; idx < HEART_PROFILE_NUM; idx++) {
        if (gatts_if == ESP_GATT_IF_NONE || gatts_if == heart_rate_profile_tab[idx].gatts_if) {
            if (heart_rate_profile_tab[idx].gatts_cb) {
                heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
            }
        }
    }
}

// Initialize Bluetooth
void BT_Init() {
    ESP_LOGI(GATTS_TABLE_TAG, "Initializing Bluetooth...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Start Bluetooth
    btStart();

    // Initialize Bluedroid
    esp_bluedroid_status_t bt_state = esp_bluedroid_get_status();
    if (bt_state == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        ESP_ERROR_CHECK(esp_bluedroid_init());
    }

    if (bt_state != ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bluedroid_enable());
    }

    // Register callbacks
    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(ESP_HEART_RATE_APP_ID));

    // Configure security parameters
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;  // No input/output capability (matching working example)
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint32_t passkey = 123456;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;

    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    Serial.println("Bluetooth initialized - Passkey: 123456");
}

// Setup WiFi with saved credentials or AP mode
void setupWiFi() {
    // Load saved WiFi credentials
    preferences.begin("wifi", false);
    stored_ssid = preferences.getString("ssid", "");
    stored_password = preferences.getString("password", "");
    preferences.end();

    // Try to connect if we have saved credentials
    if (stored_ssid.length() > 0) {
        Serial.println("Attempting to connect with saved WiFi credentials...");
        Serial.print("SSID: ");
        Serial.println(stored_ssid);

        WiFi.mode(WIFI_STA);
        WiFi.begin(stored_ssid.c_str(), stored_password.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 40) {  // 20 seconds timeout
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n========================================");
            Serial.println("WiFi connected successfully!");
            Serial.println("Access the device at:");
            Serial.print("  - http://");
            Serial.print(mdnsHostname);
            Serial.println(".local");
            Serial.print("  - http://");
            Serial.println(WiFi.localIP());
            Serial.println("========================================");

            // Start mDNS
            if (MDNS.begin(mdnsHostname)) {
                MDNS.addService("http", "tcp", 80);
                Serial.println("mDNS responder started");
            } else {
                Serial.println("Error starting mDNS");
            }

            isAPMode = false;
            return;
        } else {
            Serial.println("\nFailed to connect with saved credentials.");
        }
    }

    // Start AP mode if no credentials or connection failed
    Serial.println("Starting AP mode for configuration...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    isAPMode = true;

    // Start mDNS even in AP mode
    if (MDNS.begin(mdnsHostname)) {
        Serial.print("mDNS responder started. Access at: http://");
        Serial.print(mdnsHostname);
        Serial.println(".local");
        MDNS.addService("http", "tcp", 80);
    }

    // Start DNS server for captive portal
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    Serial.println("\n========================================");
    Serial.println("Access Point Started!");
    Serial.print("WiFi SSID: ");
    Serial.println(AP_SSID);
    Serial.print("WiFi Password: ");
    Serial.println(AP_PASSWORD);
    Serial.println("\nAccess the device at:");
    Serial.print("  - http://");
    Serial.print(mdnsHostname);
    Serial.println(".local");
    Serial.print("  - http://");
    Serial.println(WiFi.softAPIP());
    Serial.println("\nConnect to WiFi and you'll be redirected to config");
    Serial.println("========================================\n");
}

// Setup web server
void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        // If in AP mode, always redirect to WiFi config
        if (isAPMode) {
            request->redirect("/wifi");
        } else {
            request->send_P(200, "text/html", index_html);
        }
    });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"irk\":\"" + currentIRK + "\",";
        json += "\"irkReversed\":\"" + currentIRKReversed + "\",";
        json += "\"irkBase64\":\"" + currentIRKBase64 + "\",";
        json += "\"irkArray\":\"" + currentIRKArray + "\",";
        json += "\"mac\":\"" + connectedDeviceMAC + "\",";
        json += "\"irkRetrieved\":" + String(irkRetrieved ? "true" : "false") + ",";
        json += "\"isAPMode\":" + String(isAPMode ? "true" : "false") + ",";
        json += "\"ipAddress\":\"" + (isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    // WiFi Configuration page
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", wifi_html);
    });

    // Save WiFi credentials
    server.on("/api/wifi/save", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
            DynamicJsonDocument doc(256);
            deserializeJson(doc, (char*)data);

            String ssid = doc["ssid"].as<String>();
            String password = doc["password"].as<String>();

            preferences.begin("wifi", false);
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.end();

            String response = "{\"success\":true}";
            request->send(200, "application/json", response);

            delay(1000);
            ESP.restart();
        });

    // Clear WiFi credentials
    server.on("/api/wifi/clear", HTTP_POST, [](AsyncWebServerRequest *request){
        preferences.begin("wifi", false);
        preferences.clear();
        preferences.end();

        String response = "{\"success\":true}";
        request->send(200, "application/json", response);

        delay(1000);
        ESP.restart();
    });

    // WiFi status
    server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
        json += "\"ssid\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "") + "\",";
        json += "\"ip\":\"" + (isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
        json += "\"mode\":\"" + String(isAPMode ? "AP" : "Station") + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });

    // Reset IRK endpoint
    server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request){
        // Clear IRK data
        currentIRK = "No IRK retrieved yet";
        currentIRKBase64 = "";
        currentIRKReversed = "";
        currentIRKArray = "";
        connectedDeviceMAC = "None";
        irkRetrieved = false;

        // Clear bonded devices
        remove_all_bonded_devices();

        Serial.println("IRK reset requested via web interface");

        String response = "{\"success\":true}";
        request->send(200, "application/json", response);
    });

    // Captive portal handler - redirect all unknown URLs to WiFi config in AP mode
    server.onNotFound([](AsyncWebServerRequest *request){
        if (isAPMode) {
            // Redirect to WiFi config page for captive portal
            request->redirect("http://192.168.4.1/wifi");
        } else {
            request->send(404, "text/plain", "Not Found");
        }
    });

    server.begin();
    Serial.println("Web server started");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("ESP32 IRK Finder - Bluedroid Version");
    Serial.println("========================================");

    // Setup LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Configure advertising
    heart_rate_adv_config.set_scan_rsp = false;
    heart_rate_adv_config.include_txpower = true;
    heart_rate_adv_config.min_interval = 0x0006;
    heart_rate_adv_config.max_interval = 0x0010;
    heart_rate_adv_config.appearance = 0x00;
    heart_rate_adv_config.manufacturer_len = 0;
    heart_rate_adv_config.p_manufacturer_data = NULL;
    heart_rate_adv_config.service_data_len = 0;
    heart_rate_adv_config.p_service_data = NULL;
    heart_rate_adv_config.service_uuid_len = sizeof(sec_service_uuid);
    heart_rate_adv_config.p_service_uuid = sec_service_uuid;
    heart_rate_adv_config.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

    heart_rate_scan_rsp_config.set_scan_rsp = true;
    heart_rate_scan_rsp_config.include_name = true;
    heart_rate_scan_rsp_config.manufacturer_len = sizeof(test_manufacturer);
    heart_rate_scan_rsp_config.p_manufacturer_data = test_manufacturer;

    heart_rate_adv_params.adv_int_min = 0x100;
    heart_rate_adv_params.adv_int_max = 0x100;
    heart_rate_adv_params.adv_type = ADV_TYPE_IND;
    heart_rate_adv_params.own_addr_type = BLE_ADDR_TYPE_RANDOM;
    heart_rate_adv_params.channel_map = ADV_CHNL_ALL;
    heart_rate_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

    // Setup WiFi
    setupWiFi();

    // Setup web server
    setupWebServer();

    // Initialize Bluetooth
    BT_Init();

    // Don't clear bonded devices - allow re-connection to previously paired devices
    // delay(1000);
    // remove_all_bonded_devices();

    Serial.println("\n========================================");
    Serial.println("System ready!");
    Serial.println("Web interface:");
    Serial.print("  - http://");
    Serial.print(mdnsHostname);
    Serial.println(".local");
    Serial.print("  - http://");
    Serial.println(isAPMode ? WiFi.softAPIP() : WiFi.localIP());
    Serial.println("BLE Device name: ESP32_IRK_FINDER");
    Serial.println("Passkey: 123456");
    Serial.println("========================================\n");
}

void loop() {
    // Process DNS requests in AP mode
    if (isAPMode) {
        dnsServer.processNextRequest();
    }

    delay(1000);

    // Check for bonded devices periodically
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
        if (!irkRetrieved) {
            show_bonded_devices();
        }
    }
}