# Flash Wear Protection

## Problem

nRF52840 internal flash has limited write endurance:
- **Guaranteed:** 10,000 erase/write cycles per page
- **Typical:** ~100,000 cycles (but not guaranteed)

NodeDB was being saved to flash with 1-minute throttle. In active mesh networks with frequent node updates, this could wear out flash quickly.

## Analysis

### Write Frequency (Original: 1 minute throttle)

| Scenario | Writes/Day | Flash Life (256KB FS) | Flash Life (1MB FS) |
|----------|------------|----------------------|---------------------|
| 24/7 active mesh | 1,440 | ~1.2 years | ~4.9 years |
| 12h active, 50% trigger | 360 | ~4.9 years | ~19.5 years |
| Casual use | ~100 | ~17.5 years | ~70 years |

### Write Frequency (New: 10 minute throttle)

| Scenario | Writes/Day | Flash Life (256KB FS) | Flash Life (1MB FS) |
|----------|------------|----------------------|---------------------|
| 24/7 active mesh | 144 | ~12 years | ~49 years |
| 12h active, 50% trigger | 36 | ~49 years | ~195 years |
| Casual use | ~10 | ~175 years | ~700 years |

## What Gets Written

Only `SEGMENT_NODEDATABASE` is written automatically and frequently. Other segments are user-triggered:

| Segment | Trigger | Frequency |
|---------|---------|-----------|
| **NODEDATABASE** | Node updates | Auto, throttled |
| CONFIG | Menu/settings | User action (rare) |
| MODULECONFIG | Module settings | User action (rare) |
| DEVICESTATE | Boot recovery | Once at boot |
| CHANNELS | Channel changes | User action (rare) |

## Trade-off

**Before (1 min):** Max data loss on unexpected power-off = 1 minute of node updates
**After (10 min):** Max data loss on unexpected power-off = 10 minutes of node updates

This is acceptable because:
1. Node data is automatically rebuilt when nodes transmit again
2. Important data (config, channels, owner) is saved immediately on change
3. Clean shutdown (`nodeDB->saveToDisk()`) saves everything
4. Backup system provides additional protection

## Implementation

```cpp
// src/mesh/Default.h
#define TEN_MINUTES_MS 10 * 60 * 1000

// src/mesh/NodeDB.cpp:1898
if (!Throttle::isWithinTimespanMs(lastNodeDbSave, TEN_MINUTES_MS)) {
    saveToDisk(SEGMENT_NODEDATABASE);
    lastNodeDbSave = millis();
}
```

## Files Modified

| File | Change |
|------|--------|
| `src/mesh/Default.h` | Added `TEN_MINUTES_MS` constant |
| `src/mesh/NodeDB.cpp` | Changed throttle from `ONE_MINUTE_MS` to `TEN_MINUTES_MS` |

## Conclusion

10x increase in flash lifespan with minimal impact on data integrity. For devices in active mesh networks, this change extends flash life from ~1-5 years to ~12-50+ years.
