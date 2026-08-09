# Target: Seeed SenseCAP T1000-E (`tracker-t1000-e`)

Date: 2026-07-04. Status: ✅ built & verified on hardware.

**Not an InkHUD UI target** — the T1000-E is a screenless tracker. It is built from this
tree so it ships our core work (auto backup/restore) plus a fix for an upstream boot-loop.
Nothing in `src/graphics/niche/InkHUD2/` is compiled into it.

## Hardware

| Item | Value |
|---|---|
| MCU | nRF52840, SoftDevice **S140 7.3.0** |
| `hw_model` | 71 |
| LoRa | LR1110 |
| GPS | AG3335 (UART) |
| Accelerometer | QMA6100P (i2c, `HAS_QMA6100P`) |
| Screen | none |

Build: `pio run -e tracker-t1000-e` → `.pio/build/tracker-t1000-e/firmware-….uf2`

## Flashing

**DFU:** hold the button → connect USB **twice quickly** → solid green LED = DFU
(volume `T1000-E`). Drag-drop the `.uf2`; `fcopyfile … Input/output error` is **normal**
and means flashing started. Hard power-cycle: unplug USB → hold button → plug in while
holding ~3 s.

**Erase first** (official Seeed step, for a clean slate): flash
`bin/Meshtastic_nRF52_factory_erase_v3_S140_7.3.0.uf2` — it must match SoftDevice
**S140 7.3.0**. Wipes FS/config/keys/BLE bonds, then returns to DFU.
⚠️ Erase is **flash-only** — it does *not* clear a hardware-wedged i2c sensor (see below).

## Operating notes

- **BLE PIN is fixed at `123456`** — no screen to show a random one.
- ⚠️ **USB ↔ BLE contention.** A real serial PhoneAPI client (`meshtastic --info`) holds
  the device's single phone slot, so the mobile app hangs on **"Communicating"** right
  after the PIN. Fix: **unplug USB**, then connect over Bluetooth. A raw serial-log reader
  (`cat`, pyserial) does *not* block BLE — only a protocol client does.
- **Serial logs without a TTY:** macOS has no `timeout`, and `timeout cat` is not a
  substitute. Use pyserial (DTR high) or `cat` + `kill`.

## 🔴→✅ Fix: boot-loop at the i2c scan (meshtastic/firmware#4615)

**Symptom.** Boot hangs at `Scan for i2c devices` — no serial API, no BLE, because
`setup()` never returns → watchdog → bootloader → repeat.

**Root cause.** A wedged i2c sensor holds SDA low. Adafruit's nRF52
`TwoWire::endTransmission()` spins in an **unbounded `while (!EVENTS_STOPPED)`** loop, so
the very first probe hangs forever. It survives warm reset *and* a full factory erase —
the sensor is battery-powered and erase only touches flash.

**Fix** — `src/main.cpp`, guarded by `#if defined(TRACKER_T1000_E)`, before `Wire.begin()`:

1. **Sensor power-cycle** — `PIN_3V3_EN` (P1.6) + `T1000X_SENSOR_EN_PIN` (P0.4) LOW for
   120 ms → HIGH, to hard-reset a healthy-but-stuck sensor.
   ⚠️ **Gentle cut only — do NOT ground SDA/SCL during the cut.** That half-revives a
   marginal sensor into a "reads OK at idle but still hangs the TWIM" state, which defeats
   the skip in step 3.
2. **`i2cBusRecover()`** — if SDA is still low, bit-bang up to 9 SCL pulses + STOP; returns
   whether SDA was released.
3. **Skip-scan safety net** — `i2cScanner->scanPort(WIRE)` wrapped in `if (i2cBusUsable)`.
   If the bus is still wedged, skip the scan so the device boots anyway (without i2c
   sensors for that session).

**Outcome.** Healthy unit → power-cycle recovers the sensor → scan runs → accelerometer
works. Wedged unit → scan skipped → device boots fully (LoRa/GPS/BLE up). The log tells
which path was taken: `I2C bus stuck … STILL STUCK` → `skipping scan` → `Advertise`.

### ⚠️ This particular unit's QMA6100P is dead hardware

SDA stayed stuck through: the power-cycle, an aggressive depower (SDA/SCL grounded,
300 ms), a bounded bit-bang health probe, **and** a full factory erase. Not fixable in
firmware — which matches #4615, where erase also failed. Accelerometer / motion-wake are
gone **on this board only**; everything else works.

**Rejected approaches** — both let boot reach the scan and then hang the TWIM:

- grounded-line depower;
- bit-bang health probe (it passed both idle and probe, yet TWIM still hung).

Reverted to *gentle depower + skip-on-stuck-SDA*, which is reliable.

### Why MeshCore doesn't hit this

MeshCore's `variants/t1000-e` never performs a blocking i2c transaction at boot:
`Wire.begin()` is peripheral config only, its sensors are ADC (temp/light), UART (GPS) and
SPI (LoRa), the QMA6100P is unused, and there is no i2c scan. Meshtastic hangs **solely**
because of its boot-time i2c address scan — so a device that "works fine on MeshCore" tells
you nothing about whether its i2c sensor is healthy.

## Why this target matters beyond itself

The T1000-E is the reason core auto-restore exists. Backup/restore used to be reachable
**only** from the InkHUD menu or an admin message — so a screenless device that corrupted
its config had no way to self-heal. `NodeDB::loadFromDisk` now records a
`corruptSettingsMask` when a config/moduleConfig/channels load fails and calls a guarded
`restorePreferences()` afterwards, recovering from the newest good backup (a no-op on first
boot or with no backup). Both that and the i2c fix above are target-independent — they
carry no InkHUD2 dependency and would apply to any Meshtastic build — but they live in core
files (`NodeDB.cpp`, `main.cpp`), so they must be re-checked on every base bump.
