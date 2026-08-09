# Display Driver: Heltec Vision Master E290

Documentation for the Heltec Vision Master E290 display driver in the context of InkHUD2.

## Overview

**Device:** Heltec Vision Master E290
**Panel:** DEPG0290BNS800F6_V2.1
**Size:** 2.9 inch
**Resolution:** 128 x 296 pixels
**Controller:** SSD1680 (SSD16XX family)
**MCU:** ESP32-S3

## Driver Architecture

```
+-------------------------------------------------------------+
|                        InkHUD2                               |
|  +-----------------------------------------------------+    |
|  |                    InkHUD2.cpp                       |    |
|  |  - Singleton orchestration                           |    |
|  |  - OSThread for periodic updates                     |    |
|  |  - Buffer -> Driver coordination                     |    |
|  +------------------------+----------------------------+    |
|                           |                                  |
|  +------------------------v----------------------------+    |
|  |                  DisplayDriver                       |    |
|  |  (abstract interface)                                |    |
|  |  - init(), width(), height()                         |    |
|  |  - busy(), update(), sleep(), wake()                 |    |
|  +------------------------+----------------------------+    |
|                           |                                  |
|  +------------------------v----------------------------+    |
|  |                   EInkAdapter                        |    |
|  |  (Drivers/EInkAdapter.h)                             |    |
|  |  - Adapter between InkHUD2 and NicheGraphics drivers |    |
|  |  - Converts DisplayDriver calls -> EInk              |    |
|  +------------------------+----------------------------+    |
+---------------------------+----------------------------------+
                            |
+---------------------------v----------------------------------+
|                   NicheGraphics::Drivers                     |
|  +-----------------------------------------------------+    |
|  |                      EInk                            |    |
|  |  (Drivers/EInk/EInk.h)                               |    |
|  |  - Base class for all E-Ink drivers                  |    |
|  |  - OSThread for async polling                        |    |
|  |  - UpdateTypes: FULL, FAST                           |    |
|  +------------------------+----------------------------+    |
|                           |                                  |
|  +------------------------v----------------------------+    |
|  |                    SSD16XX                           |    |
|  |  (Drivers/EInk/SSD16XX.h)                            |    |
|  |  - Base class for SSD1680/SSD1681 controllers        |    |
|  |  - SPI communication                                 |    |
|  |  - Memory management (new/old image)                 |    |
|  |  - configFullscreen(), writeNewImage(), writeOldImage()|  |
|  +------------------------+----------------------------+    |
|                           |                                  |
|  +------------------------v----------------------------+    |
|  |                 HeltecVME290                         |    |
|  |  (Drivers/EInk/HeltecVME290.h)                       |    |
|  |  - VM-E290 specific configuration                    |    |
|  |  - OTP LUT (from controller memory)                  |    |
|  |  - Buffer offset: 1 byte                             |    |
|  +-----------------------------------------------------+    |
+-------------------------------------------------------------+
```

## Key Files

| File | Purpose |
|------|---------|
| `InkHUD2/Drivers/EInkAdapter.h` | Adapter InkHUD2 <-> NicheGraphics |
| `InkHUD2/Core/Buffer.h` | Frame buffer with rotation support |
| `Drivers/EInk/EInk.h/.cpp` | E-Ink base class |
| `Drivers/EInk/SSD16XX.h/.cpp` | SSD16XX base class |
| `Drivers/EInk/HeltecVME290.h/.cpp` | Driver for VM-E290 |
| `variants/.../nicheGraphics.h` | Device-specific initialization |

## Buffer (Frame Buffer)

```cpp
// Create buffer
Buffer buffer(driverWidth, driverHeight, rotation);

// Data format:
// - 1 bit per pixel (monochrome)
// - 8 pixels per byte
// - MSB = leftmost pixel
// - 0 = BLACK (ink on), 1 = WHITE (no ink)

// Rotation:
// 0 = 0deg, 1 = 90deg CW, 2 = 180deg, 3 = 270deg CW

// Logical coordinates are automatically converted
// to physical driver coordinates
buffer.setPixel(x, y, Color::BLACK);
```

## SSD16XX: Differential Refresh

SSD16XX E-Ink controllers support **differential refresh** (partial update):

```
+----------------+     +----------------+
|   Old Memory   |     |   New Memory   |
|   (reg 0x26)   |     |   (reg 0x24)   |
|                |     |                |
|   Previous     |     |   New          |
|   image        |     |   image        |
+-------+--------+     +-------+--------+
        |                      |
        +----------+-----------+
                   |
                   v
        +---------------------+
        |   E-Ink Controller  |
        |                     |
        |  Compares old/new   |
        |  Updates only       |
        |  changed pixels     |
        +---------------------+
```

### FULL vs FAST refresh

| Type | Time | Ghosting | When to use |
|------|------|----------|-------------|
| FULL | ~2 sec | None | Boot, orientation change, periodic cleanup |
| FAST | ~300 ms | Possible | Normal UI updates |

```cpp
// In HeltecVME290.cpp
void HeltecVME290::configUpdateSequence() {
    switch (updateType) {
    case FAST:
        sendCommand(0x22);
        sendData(0xFF);    // Differential refresh (mode 2)
        break;
    case FULL:
        sendCommand(0x22);
        sendData(0xF7);    // Full refresh
        break;
    }
}
```

## OTP LUT (Look-Up Table)

VM-E290 uses **OTP LUT** - waveform tables stored in controller memory:

```cpp
void HeltecVME290::configWaveform() {
    sendCommand(0x3C); // Border waveform
    sendData(0x05);    // Follow LUT1 (white border)

    sendCommand(0x18); // Temperature sensor
    sendData(0x80);    // Internal sensor for automatic waveform selection
}
```

**Advantages of OTP LUT:**
- No need to store LUT in firmware (~200 bytes savings)
- Automatic temperature adaptation
- Optimized by panel manufacturer

## Async Update

E-Ink updates are performed asynchronously:

```cpp
// 1. Start update
driver->update(buffer, FAST);  // Non-blocking call

// 2. Polling in OSThread
bool SSD16XX::isUpdateDone() {
    return digitalRead(pin_busy) == LOW;
}

// 3. Finalization after completion
void SSD16XX::finalizeUpdate() {
    if (updateType != FULL) {
        writeNewImage();   // Copy new -> old
        writeOldImage();
        sendCommand(0x7F); // Terminate without update
    }
    deepSleep();           // Power saving
}
```

### Race Condition Prevention

```cpp
// In InkHUD2::runOnce()
if (pipeNeedsUpdate || fullRefreshRequested) {
    // CRITICAL: Don't render while display is busy!
    // Buffer must not change during finalizeUpdate()
    if (driver->busy()) {
        return 50;  // Retry in 50ms
    }
    render();
}
```

## Initialization (nicheGraphics.h)

```cpp
void setupNicheGraphics() {
    // 1. Power on display
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(10);

    // 2. Initialize SPI (HSPI on ESP32)
    SPIClass* hspi = new SPIClass(HSPI);
    hspi->begin(PIN_EINK_SCLK, -1, PIN_EINK_MOSI, PIN_EINK_CS);

    // 3. Create driver
    Drivers::EInk* driver = new Drivers::HeltecVME290;
    driver->begin(hspi, PIN_EINK_DC, PIN_EINK_CS, PIN_EINK_BUSY, PIN_EINK_RES);

    // 4. Configure InkHUD2
    InkHUD2::Config config;
    config.defaultRotation = 3;  // 270deg - LoRa antenna up
    // ... buttons, backlight ...

    // 5. Start InkHUD2
    InkHUD2::setup(driver, config);
}
```

## Pins (VM-E290)

| Pin | GPIO | Purpose |
|-----|------|---------|
| SCLK | 3 | SPI Clock |
| MOSI | 2 | SPI Data |
| CS | 5 | Chip Select |
| DC | 4 | Data/Command |
| BUSY | 6 | Busy signal |
| RES | 1 | Reset |
| VEXT | 45 | Power enable |

## Driver Creation History

HeltecVME290 is a **hybrid driver** built from two existing drivers:

### Problem

Initially, the `DEPG0290BNS800` driver (same size panel from DKE) was used for VM-E290.
Result: **severe ghosting** during FAST refresh.

Cause: DEPG0290BNS800 uses **custom LUT** - a waveform table embedded in firmware code.
This table was optimized for the specific DKE panel and was not suitable for the Heltec panel.

### Solution

Adopted the approach from `ZJY128296_029EAAMFGN` driver (WeActStudio 2.9" panel):
- **OTP LUT** - waveform from controller memory (optimized by panel manufacturer)
- **Internal temperature sensor** - automatic waveform selection based on temperature
- **Border waveform 0x05** - screen border follows LUT1

But kept from DEPG0290BNS800:
- **Buffer offset 1 byte** - VM-E290 panel wiring specifics
- **No configScanning** - ZJY overrode scanning, VM-E290 doesn't need this

### Driver Comparison

| Parameter | DEPG0290BNS800 | ZJY128296 | HeltecVME290 |
|-----------|----------------|-----------|--------------|
| LUT | Custom (in code) | OTP | **OTP** |
| Voltages | Custom 15V/-15V | Default | **Default** |
| Border | 0x60 (hold) | 0x05 (LUT1) | **0x05** |
| Temp sensor | None | 0x80 | **0x80** |
| Update seq FAST | 0xCF | 0xFF | **0xFF** |
| Buffer offset | 1 byte | 0 | **1 byte** |
| configScanning | No | Yes | **No** |
| FAST timing | 450ms | 300ms | **300ms** |
| FULL timing | 3000ms | 2000ms | **2000ms** |

### Result

Ghosting eliminated. OTP LUT automatically adapts to temperature and is optimized for the specific panel.

---

## VM-E290 Specifics

### Buffer Offset

```cpp
HeltecVME290() : SSD16XX(width, height, supported, 1) {}
//                                                  ^ offset 1 byte
```

The panel requires a 1 byte offset due to internal wiring specifics.

### Elongated Screen

Aspect ratio 128:296 (~1:2.3) affects UI:
- Smaller fonts are used
- Adaptive node card height
- 2 rotation options (0deg/180deg vs all 4)

```cpp
bool isElongated = (maxDim * 10 / minDim > 15);  // >1.5 ratio
```

## Timings

| Operation | Time | Polling interval |
|-----------|------|------------------|
| FULL refresh | ~2000 ms | 100 ms |
| FAST refresh | ~300 ms | 50 ms |
| Deep sleep entry | ~10 ms | - |

## Power Consumption

- **During update:** ~15-20 mA
- **Deep sleep:** ~1-5 uA
- **Between updates:** Automatic deep sleep

## Debugging

### Serial output

```
[NicheGfx] setupNicheGraphics() start
[InkHUD2] setup() start
[InkHUD2] setup() complete
[NicheGfx] setupNicheGraphics() complete
```

### Common Issues

| Symptom | Cause | Solution |
|---------|-------|----------|
| White screen | VEXT not enabled | Check `digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE)` |
| Ghosting | Race condition | Check `driver->busy()` before render |
| Inverted colors | Wrong buffer format | BLACK=0, WHITE=1 |
| Shifted image | Wrong offset | Check `bufferOffsetX` |

## Related Documents

- `ARCHITECTURE.md` - InkHUD2 general architecture
- `ARCHITECTURE_SIMPLE.md` - Simplified description
- `../../../Drivers/EInk/` - Other E-Ink drivers for reference
