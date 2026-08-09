# HowTo: Add a New E-Ink Driver

Guide for adding a new E-Ink driver for InkHUD (legacy) and InkHUD2.

## File Structure

```
src/graphics/niche/Drivers/EInk/
├── EInk.h              # Base class (abstract)
├── EInk.cpp
├── SSD16XX.h           # Base class for SSD1680/SSD1681
├── SSD16XX.cpp
├── HeltecVME290.h      # Specific driver (example)
├── HeltecVME290.cpp
└── ...
```

## Step 1: Create Header File

`src/graphics/niche/Drivers/EInk/MyNewDisplay.h`:

```cpp
#pragma once

#ifdef MESHTASTIC_INCLUDE_NICHE_GRAPHICS

#include "configuration.h"
#include "./SSD16XX.h"

namespace NicheGraphics::Drivers
{

class MyNewDisplay : public SSD16XX
{
private:
    // Panel characteristics
    static constexpr uint32_t width = 128;
    static constexpr uint32_t height = 296;
    static constexpr UpdateTypes supported = (UpdateTypes)(FULL | FAST);

public:
    // bufferOffsetX: buffer offset in bytes (if panel requires it)
    MyNewDisplay() : SSD16XX(width, height, supported, 0) {}

protected:
    // Override required methods:
    void configWaveform() override;
    void configUpdateSequence() override;
    void detachFromUpdate() override;

    // Optional:
    // void configScanning() override;
    // void configVoltages() override;
    // void finalizeUpdate() override;
};

} // namespace NicheGraphics::Drivers

#endif
```

## Step 2: Create Implementation File

`src/graphics/niche/Drivers/EInk/MyNewDisplay.cpp`:

```cpp
#include "./MyNewDisplay.h"

#ifdef MESHTASTIC_INCLUDE_NICHE_GRAPHICS

using namespace NicheGraphics::Drivers;

void MyNewDisplay::configWaveform()
{
    // Option A: OTP LUT (recommended if panel supports it)
    sendCommand(0x3C); // Border waveform
    sendData(0x05);    // Follow LUT1 (drive white)

    sendCommand(0x18); // Temperature sensor
    sendData(0x80);    // Internal sensor for automatic waveform selection

    // Option B: Custom LUT (if OTP causes ghosting)
    // See DEPG0290BNS800.cpp for example
}

void MyNewDisplay::configUpdateSequence()
{
    switch (updateType) {
    case FAST:
        sendCommand(0x22);
        sendData(0xFF);    // OTP differential (mode 2)
        // or 0xCF for custom LUT
        break;

    case FULL:
    default:
        sendCommand(0x22);
        sendData(0xF7);    // OTP full refresh
        break;
    }
}

void MyNewDisplay::detachFromUpdate()
{
    switch (updateType) {
    case FAST:
        return beginPolling(50, 300);   // interval 50ms, expected ~300ms
    case FULL:
    default:
        return beginPolling(100, 2000); // interval 100ms, expected ~2s
    }
}

#endif
```

## Step 3: Connect Driver to Device Variant

Edit `variants/<arch>/<device>/nicheGraphics.h`:

### For Legacy InkHUD:

```cpp
#include "graphics/niche/Drivers/EInk/MyNewDisplay.h"

void setupNicheGraphics()
{
    using namespace NicheGraphics;

    // SPI
    SPIClass *hspi = new SPIClass(HSPI);
    hspi->begin(PIN_EINK_SCLK, -1, PIN_EINK_MOSI, PIN_EINK_CS);

    // Driver
    Drivers::EInk *driver = new Drivers::MyNewDisplay;
    driver->begin(hspi, PIN_EINK_DC, PIN_EINK_CS, PIN_EINK_BUSY, PIN_EINK_RES);

    // InkHUD
    InkHUD::InkHUD *inkhud = InkHUD::InkHUD::getInstance();
    inkhud->setDriver(driver);

    // ... remaining setup
}
```

### For InkHUD2:

```cpp
#include "graphics/niche/Drivers/EInk/MyNewDisplay.h"
#include "graphics/niche/InkHUD2/Setup.h"

void setupNicheGraphics()
{
    using namespace NicheGraphics;

    // Power on
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(10);

    // SPI
    SPIClass* hspi = new SPIClass(HSPI);
    hspi->begin(PIN_EINK_SCLK, -1, PIN_EINK_MOSI, PIN_EINK_CS);

    // Driver
    Drivers::EInk* driver = new Drivers::MyNewDisplay;
    driver->begin(hspi, PIN_EINK_DC, PIN_EINK_CS, PIN_EINK_BUSY, PIN_EINK_RES);

    // InkHUD2
    InkHUD2::Config config;
    config.defaultRotation = 3;
    // ... button setup

    InkHUD2::setup(driver, config);
}
```

## Key Parameters

### Buffer Offset

```cpp
MyNewDisplay() : SSD16XX(width, height, supported, 1) {}
//                                                  ^
// Some panels require buffer offset due to internal wiring.
// If image is shifted - try 1.
```

### Update Sequence Commands

| Code | Description |
|------|-------------|
| 0xF7 | FULL refresh, OTP LUT |
| 0xFF | FAST refresh (differential), OTP LUT |
| 0xCF | FAST refresh, custom LUT (from registers) |

### Border Waveform

| Code | Description |
|------|-------------|
| 0x05 | Follow LUT1 (white border) |
| 0x60 | Actively hold during update |

### Polling Timings

```cpp
beginPolling(interval, expectedDuration);
// interval: how often to check busy pin (ms)
// expectedDuration: minimum time before starting to check (ms)
```

Typical values:
- FAST: 50ms interval, 200-500ms expected
- FULL: 100ms interval, 1500-3000ms expected

## Debugging

### White Screen
- Check power (VEXT)
- Check SPI pins
- Check reset pin

### Ghosting (residual image)
- Try OTP LUT instead of custom
- Add temperature sensor (0x18, 0x80)
- Increase time between updates

### Shifted Image
- Try bufferOffsetX = 1

### Inverted Colors
- Check buffer format (0=BLACK, 1=WHITE)
- Some panels may require configScanning override

## Reference Drivers

| Driver | Features |
|--------|----------|
| `HeltecVME290` | OTP LUT, offset 1, simple |
| `DEPG0290BNS800` | Custom LUT, voltages, optimized finalizeUpdate |
| `ZJY128296_029EAAMFGN` | OTP LUT, configScanning override |
| `GDEY0154D67` | 1.54" square, different scanning |

## Checklist

- [ ] Created .h file with correct dimensions and supported types
- [ ] Created .cpp with configWaveform, configUpdateSequence, detachFromUpdate
- [ ] Added include to variant's nicheGraphics.h
- [ ] Replaced driver in setupNicheGraphics()
- [ ] Passed reset pin to begin() if used
- [ ] Build verified
- [ ] Tested on device (no ghosting, correct orientation)
