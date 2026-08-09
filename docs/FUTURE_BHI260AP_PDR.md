# BHI260AP PDR Integration (Future Feature)

## Overview

T-Echo Plus has BHI260AP IMU which supports Pedestrian Dead Reckoning (PDR) with GPS fusion. Currently unused because Meshtastic doesn't have a driver for it.

## What PDR Provides

- **GPS duty-cycling**: GPS can be turned off periodically to save power
- **Position tracking**: IMU continues tracking position when GPS is off
- **Drift correction**: When GPS turns back on, it corrects accumulated drift
- **Better accuracy**: Fusion of IMU + GPS gives smoother, more accurate position

## Hardware

- **Chip**: Bosch BHI260AP (6-axis IMU: accelerometer + gyroscope)
- **I2C Address**: 0x28
- **Already on T-Echo Plus**: Yes, defined in `variant.h` as `HAS_BHI260AP`

## Available Resources

### SDK (Open Source, BSD-3-Clause)
- Official: https://github.com/boschsensortec/BHI2xy_SensorAPI
- nRF52 port: https://github.com/robcazzaro/nRF52-BHY2-Sensor-API
- nRF Connect SDK: https://github.com/dariosortino/nRFConnect-BHY2-Sensor-API

### Documentation
- Datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bhi260ap-ds000.pdf
- BSX Fusion App Note: https://www.bosch-sensortec.com/media/boschsensortec/downloads/application_notes_1/bst-bhi260_bhi360-an002.pdf
- FAQ: https://community.bosch-sensortec.com/knowledge-base-pg631enp/post/faqs---bhi260ap-icnnWQcgB85Gh3w

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Meshtastic                           │
│                                                         │
│  ┌─────────────┐     ┌─────────────┐     ┌───────────┐ │
│  │  GPS Thread │────▶│ PDR Manager │────▶│ Position  │ │
│  │   (L76K)    │     │  (new)      │     │  Module   │ │
│  └─────────────┘     └──────┬──────┘     └───────────┘ │
│                             │                           │
└─────────────────────────────┼───────────────────────────┘
                              │
                              ▼
                    ┌─────────────────┐
                    │    BHI260AP     │
                    │                 │
                    │  ┌───────────┐  │
                    │  │ PDR Algo  │  │
                    │  │ (Bosch)   │  │
                    │  └───────────┘  │
                    │                 │
                    │  ┌───────────┐  │
                    │  │  6-axis   │  │
                    │  │   IMU     │  │
                    │  └───────────┘  │
                    └─────────────────┘
```

## Data Flow

1. **GPS → BHI260AP**: Send current GPS coordinates when available
2. **BHI260AP internal**: PDR algorithm fuses GPS + IMU data
3. **BHI260AP → Host**: Read corrected position (or use GPS position directly when fresh)
4. **Duty cycle**: GPS can sleep for X seconds, BHI260AP tracks position via IMU

## Implementation Plan

### Phase 1: BHI260AP Driver
- [ ] Create `src/motion/BHI260APSensor.h/.cpp`
- [ ] Initialize BHI260AP via I2C
- [ ] Load PDR firmware into BHI260AP
- [ ] Basic communication (read IMU data)

### Phase 2: PDR Integration
- [ ] Create `src/gps/PDRManager.h/.cpp`
- [ ] Feed GPS coordinates to BHI260AP
- [ ] Read PDR-corrected position
- [ ] Handle GPS duty-cycling logic

### Phase 3: Meshtastic Integration
- [ ] Modify `PositionModule` to use PDR when available
- [ ] Add config options for PDR enable/disable
- [ ] Add config for GPS duty-cycle interval

### Phase 4: Power Optimization
- [ ] Measure power savings with duty-cycling
- [ ] Tune duty-cycle intervals
- [ ] Test indoor/outdoor scenarios

## Key Files to Create/Modify

```
src/
├── motion/
│   └── BHI260APSensor.h/.cpp    # NEW: BHI260AP driver
├── gps/
│   └── PDRManager.h/.cpp        # NEW: PDR fusion manager
└── modules/
    └── PositionModule.cpp       # MODIFY: Use PDR position
```

## Estimated Effort

- **Phase 1**: 2-3 days (driver basics)
- **Phase 2**: 3-5 days (PDR integration)
- **Phase 3**: 1-2 days (Meshtastic integration)
- **Phase 4**: 1-2 days (testing & tuning)

**Total**: ~1-2 weeks

## Notes

- BHI260AP has no magnetometer, so no compass functionality
- PDR without GPS correction drifts heavily (seconds to lose accuracy)
- Main benefit is power saving via GPS duty-cycling, not standalone positioning
- Bosch SDK is well-documented with examples

## References

- T-Echo Plus variant.h: `#define HAS_BHI260AP`
- I2C pins: SDA=P0.26, SCL=P0.27
- Current AccelerometerThread doesn't support BHI260AP (goes to `default: disable()`)
