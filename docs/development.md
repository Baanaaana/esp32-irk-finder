# Development Guide

[← Back to README](../README.md)

## Table of Contents
- [Development Environment](#development-environment)
- [Project Structure](#project-structure)
- [Building and Testing](#building-and-testing)
- [Contributing](#contributing)
- [Creating Custom Builds](#creating-custom-builds)
- [Troubleshooting Development Issues](#troubleshooting-development-issues)

---

## Development Environment

### Required Tools

1. **PlatformIO Core or IDE**
   - VS Code with PlatformIO extension (recommended)
   - Or standalone PlatformIO Core

2. **Git**
   - For version control
   - Clone the repository

3. **Python 3.6+**
   - Required by PlatformIO
   - For build scripts

4. **USB Drivers**
   - CP210x drivers for most ESP32 boards
   - CH340 drivers for some clone boards

### Setting Up Development Environment

1. **Clone Repository**
```bash
git clone https://github.com/yourusername/esp32-irk-finder.git
cd esp32-irk-finder
```

2. **Install PlatformIO**
```bash
# Using pip
python3 -m pip install -U platformio

# Or using homebrew (macOS)
brew install platformio
```

3. **Install Dependencies**
```bash
# PlatformIO will auto-install dependencies on first build
pio lib install
```

4. **Verify Setup**
```bash
pio --version
pio boards esp32
```

---

## Project Structure

```
esp32-irk-finder/
├── src/
│   └── main.cpp              # Main application code
├── include/
│   └── config.h              # Configuration definitions
├── lib/                      # Project-specific libraries
├── docs/                     # Documentation
│   ├── installation.md
│   ├── usage.md
│   ├── api.md
│   ├── configuration.md
│   ├── technical.md
│   └── development.md
├── releases/                 # Prebuilt firmware binaries
│   ├── esp32-irk-finder.bin
│   ├── esp32-s3-irk-finder.bin
│   ├── esp32-c3-irk-finder.bin
│   └── README.md
├── test/                     # Unit tests
├── platformio.ini            # Build configuration
├── .gitignore
└── README.md
```

### Key Files

- **src/main.cpp** - Core application logic
  - BLE implementation (Bluedroid)
  - Web server routes
  - WiFi management
  - IRK processing

- **platformio.ini** - Build configuration
  - Board definitions
  - Library dependencies
  - Build flags
  - Environment settings

- **include/config.h** - User configuration
  - Default passwords
  - Pin assignments
  - Feature flags

---

## Building and Testing

### Build Commands

**Build for specific board:**
```bash
# ESP32
pio run -e esp32dev

# ESP32-S3
pio run -e esp32s3

# ESP32-C3
pio run -e esp32c3
```

**Build all environments:**
```bash
pio run
```

**Clean build:**
```bash
pio run -t clean
pio run
```

### Upload Firmware

**Upload to connected board:**
```bash
# Auto-detect environment
pio run -t upload

# Specific environment
pio run -e esp32dev -t upload
```

**Upload with specific port:**
```bash
pio run -t upload --upload-port /dev/ttyUSB0
```

### Serial Monitor

**Monitor output:**
```bash
pio device monitor

# With specific baud rate
pio device monitor -b 115200

# With port specification
pio device monitor -p /dev/ttyUSB0
```

**Monitor with filters:**
```bash
# Add timestamp
pio device monitor -f time

# Colorize output
pio device monitor -f colorize

# Multiple filters
pio device monitor -f time -f colorize
```

### Testing

**Run unit tests:**
```bash
pio test
```

**Test specific environment:**
```bash
pio test -e esp32dev
```

**Verbose testing:**
```bash
pio test -v
```

### Debugging

**Enable verbose build:**
```bash
pio run -v
```

**Debug with GDB (requires debug probe):**
```bash
pio debug
```

**Check memory usage:**
```bash
pio run -e esp32dev -t size
```

---

## Contributing

### Code Style

**C++ Style Guidelines:**
- Use camelCase for variables and functions
- Use UPPER_CASE for constants and defines
- 4 spaces indentation (no tabs)
- Opening braces on same line

```cpp
void exampleFunction() {
    const int MAX_RETRIES = 3;
    int retryCount = 0;

    if (condition) {
        // Code here
    }
}
```

### Adding New Features

1. **Create Feature Branch**
```bash
git checkout -b feature/your-feature-name
```

2. **Implement Feature**
   - Add code to appropriate sections
   - Update configuration if needed
   - Add API endpoints if required

3. **Test Thoroughly**
   - Test on actual hardware
   - Verify all existing features still work
   - Check memory usage

4. **Update Documentation**
   - Update relevant .md files
   - Add API documentation if needed
   - Update README if feature is user-facing

5. **Submit Pull Request**
   - Clear description of changes
   - Include test results
   - Reference any issues

### Common Modifications

**Adding New BLE Service:**
```cpp
// Define UUIDs
#define CUSTOM_SERVICE_UUID 0x1234
#define CUSTOM_CHAR_UUID    0x5678

// Add to GATT database
static esp_gatts_attr_db_t gatt_db[] = {
    // ... existing services

    // Custom Service Declaration
    [IDX_CUSTOM_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid},
        ESP_GATT_PERM_READ,
        sizeof(uint16_t),
        sizeof(custom_service_uuid),
        (uint8_t *)&custom_service_uuid
    },
    // ... characteristics
};
```

**Adding New Web Endpoint:**
```cpp
// In setupWebServer()
server.on("/api/custom", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<256> doc;
    doc["status"] = "custom";
    doc["value"] = customValue;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});
```

**Adding Configuration Option:**
```cpp
// In config.h
#define CUSTOM_OPTION_ENABLED true
#define CUSTOM_TIMEOUT_MS 5000

// In main.cpp
#ifdef CUSTOM_OPTION_ENABLED
    // Custom code here
#endif
```

---

## Creating Custom Builds

### Custom Partition Table

1. **Create partitions.csv:**
```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x6000
phy_init, data, phy,     0xf000,  0x1000
factory,  app,  factory, 0x10000, 0x200000
storage,  data, spiffs,  0x210000,0x1F0000
```

2. **Update platformio.ini:**
```ini
board_build.partitions = partitions.csv
```

### Custom Build Flags

**Add feature flags:**
```ini
build_flags =
    ${env.build_flags}
    -DCUSTOM_FEATURE=1
    -DDEBUG_LEVEL=2
    -DUSE_CUSTOM_DNS
```

**Conditional compilation:**
```cpp
#if CUSTOM_FEATURE
    Serial.println("Custom feature enabled");
#endif
```

### Creating Release Binaries

**Build release version:**
```bash
# Clean build
pio run -t clean

# Build with release flags
pio run -e esp32dev

# Binary location
# .pio/build/esp32dev/firmware.bin
```

**Create merged binary (for ESP Web Tools):**
```bash
# ESP32
esptool.py --chip esp32 merge_bin \
  -o esp32-irk-finder.bin \
  --flash_mode dio \
  --flash_size 4MB \
  0x1000 .pio/build/esp32dev/bootloader.bin \
  0x8000 .pio/build/esp32dev/partitions.bin \
  0x10000 .pio/build/esp32dev/firmware.bin

# ESP32-S3
esptool.py --chip esp32s3 merge_bin \
  -o esp32-s3-irk-finder.bin \
  --flash_mode dio \
  --flash_size 8MB \
  0x0 .pio/build/esp32s3/bootloader.bin \
  0x8000 .pio/build/esp32s3/partitions.bin \
  0x10000 .pio/build/esp32s3/firmware.bin

# ESP32-C3
esptool.py --chip esp32c3 merge_bin \
  -o esp32-c3-irk-finder.bin \
  --flash_mode dio \
  --flash_size 4MB \
  0x0 .pio/build/esp32c3/bootloader.bin \
  0x8000 .pio/build/esp32c3/partitions.bin \
  0x10000 .pio/build/esp32c3/firmware.bin
```

---

## Troubleshooting Development Issues

### Common Build Errors

**Platform not installed:**
```bash
# Install platform
pio platform install espressif32

# Update platform
pio platform update
```

**Library not found:**
```bash
# Install specific library
pio lib install "ESPAsyncWebServer"

# Update all libraries
pio lib update
```

**Out of memory:**
```ini
# Use minimal partition scheme
board_build.partitions = huge_app.csv

# Or reduce debug level
build_flags = -DCORE_DEBUG_LEVEL=0
```

### Upload Issues

**Permission denied (Linux/macOS):**
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER

# Or use sudo (temporary fix)
sudo pio run -t upload
```

**Port busy:**
```bash
# Find process using port
lsof /dev/ttyUSB0

# Kill process
kill -9 <PID>
```

**Wrong upload speed:**
```ini
# In platformio.ini
upload_speed = 115200  # Try lower speed
```

### Debugging Tips

**Enable verbose logging:**
```cpp
// In main.cpp
#define DEBUG 1
#define CORE_DEBUG_LEVEL 5
```

**Add debug prints:**
```cpp
Serial.printf("[%lu] Debug: %s\n", millis(), message);
```

**Monitor free heap:**
```cpp
Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
```

**Check stack usage:**
```cpp
Serial.printf("Stack watermark: %d\n",
              uxTaskGetStackHighWaterMark(NULL));
```

### Performance Optimization

**Reduce binary size:**
```ini
build_flags =
    -Os  # Optimize for size
    -DCORE_DEBUG_LEVEL=0  # Disable debug
```

**Increase CPU frequency:**
```cpp
setCpuFrequencyMhz(240);  // Max frequency
```

**Optimize loops:**
```cpp
// Use local variables
int len = strlen(buffer);
for (int i = 0; i < len; i++) {
    // Process
}
```

---

## Resources

### Documentation
- [ESP32 Arduino Core](https://docs.espressif.com/projects/arduino-esp32/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/)

### Tools
- [ESP32 Exception Decoder](https://github.com/me-no-dev/EspExceptionDecoder)
- [PlatformIO Inspector](https://docs.platformio.org/en/latest/plus/pio-check.html)
- [Serial Monitor Pro](https://www.eltima.com/products/serial-monitor/)

### Community
- [PlatformIO Community](https://community.platformio.org/)
- [ESP32 Forum](https://www.esp32.com/)
- [Arduino Forum ESP32 Section](https://forum.arduino.cc/c/hardware/esp/67)

---

[← Back to README](../README.md)