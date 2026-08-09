# OCV Curve Configuration for nRF52840 Devices

## Problem

The default OCV (Open Circuit Voltage) curve in Meshtastic firmware has a minimum voltage of **3100mV (3.1V)**:

```cpp
// Default from power.h
#define OCV_ARRAY 4190, 4050, 3990, 3890, 3800, 3720, 3630, 3530, 3420, 3300, 3100
//            100%   90%   80%   70%   60%   50%   40%   30%   20%   10%    0%
```

This is problematic for nRF52840-based devices because:

1. **DC-DC Converter**: Most board designs use DC-DC converters that require minimum input voltage around 3.3V for stable 3.3V output
2. **E-ink Displays**: E-ink controllers typically need stable 3.3V supply
3. **LoRa Module (SX1262)**: While spec allows 1.8V minimum, in practice operates better with stable supply
4. **Flash Memory**: QSPI flash requires stable voltage for reliable operation
5. **Peripheral ICs**: RTC, sensors, and other ICs may malfunction below 3.3V

When battery voltage drops below 3.3V, the device may experience:
- Random resets
- Flash corruption
- Display glitches
- LoRa transmission failures
- I2C communication errors

---

## Solution

Set minimum OCV to **3400mV (3.4V)** to ensure safe shutdown before voltage drops too low.

### Modified OCV Curve

```cpp
#define OCV_ARRAY 4190, 4050, 3990, 3890, 3800, 3720, 3630, 3530, 3480, 3440, 3400
//            100%   90%   80%   70%   60%   50%   40%   30%   20%   10%    0%
```

**Changes from default:**
- 20%: 3420 → 3480 mV
- 10%: 3300 → 3440 mV
- 0%: 3100 → 3400 mV

This provides ~100mV safety margin above the critical 3.3V threshold.

---

## Affected Devices

All nRF52840/RAK and other devices.

---

## How It Works

The firmware uses the OCV curve for two purposes:

### 1. Battery Percentage Estimation

```cpp
// Power.cpp - interpolates between OCV points
for (int i = 0; i < NUM_OCV_POINTS; i++) {
    if (OCV[i] <= voltage) {
        // interpolate between OCV[i] and OCV[i-1]
        battery_SOC = ...;
        break;
    }
}
```

### 2. Low Voltage Shutdown

```cpp
// Power.cpp - triggers deep sleep when voltage < OCV[10] (minimum)
if (batteryLevel->getBattVoltage() < OCV[NUM_OCV_POINTS - 1]) {
    low_voltage_counter++;
    if (low_voltage_counter > 10) {
        powerFSM.trigger(EVENT_LOW_BATTERY);  // deep sleep
    }
}
```

With minimum set to 3400mV, device will enter deep sleep while there's still enough voltage for safe operation and data preservation.

---

## Adding to Other Devices

To apply this fix to any nRF52840/RAK device, add to its `variant.h`:

```cpp
#define OCV_ARRAY 4190, 4050, 3990, 3890, 3800, 3720, 3630, 3530, 3480, 3440, 3400
```

---

## Technical Background

### LiPo/Li-ion Discharge Curve

Typical single-cell LiPo discharge characteristics:
- **4.2V**: Fully charged
- **3.7V**: Nominal voltage (50% capacity)
- **3.4V**: ~10-15% remaining
- **3.3V**: ~5% remaining (critical for 3.3V systems)
- **3.0V**: Cutoff voltage (0%, risk of damage)
- **2.5V**: Danger zone (permanent damage)

### nRF52840 Specifications

- Operating voltage: 1.7V to 5.5V (MCU itself)
- Internal DC-DC: Can operate down to 1.8V
- **But**: External components (regulators, flash, display) typically need 3.3V

### Conservative Approach

Setting 0% at 3.4V means:
- Device shows 0% battery with ~5-10% actual capacity remaining
- This capacity is reserved for safe shutdown
- Better to show 0% early than to crash at 3%
