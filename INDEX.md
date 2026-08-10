# INDEX — InkHUD2 firmware (code)

> **Read FIRST.** InkHUD2 — a niche e-ink UI for Meshtastic, targeting all supported e-ink devices (nRF52840 + ESP32-S3). This is the navigator + operational handbook; deep detail lives in code, `ARCHITECTURE.md`, and linked docs.
> **Last updated:** 2026-08-10. Recent: **rebased onto fresh upstream `6eac181`** (root-fixed the T3-S3
> radio-reconfigure crash — old base had drifted behind upstream radio/NodeDB refactors; all 9 envs build,
> T3-S3 + T1000-E HW-verified — see `REBASE_STATUS.md`); **Qi2 env split into 10000/5000mAh** (correct
> battery-% calibration via `HELTEC_MESH_POCKET_BATTERY_*`; shared base section); **T3-S3 e-paper target**
> (`tlora-t3s3-epaper-inkhud2`) + shutdown-freeze fix (`waitUntilIdle`); **Alerts fixed** — the per-channel/
> DM notification toggles were a dead stub (read nowhere); Events now reads `alertsEnabled` and only
> forces a refresh when the channel/DM alert is on. (HW/version facts are point-in-time — verify against
> current code before asserting.)

## 🔴 Protobuf drift after 2.8.0 base bump — FIXED 2026-07-18
InkHUD2 **did not compile at all** (any target) after the base was updated to Meshtastic
2.8.0 on 2026-06-16; nobody built it until the M1 port exposed it. Core fixes:
- `NodeInfoLite` **flattened**: `user.short_name/long_name` + `has_user` → `node->short_name` / `node->long_name` (presence = `[0] != '\0'`).
- `is_favorite` → bit in `bitfield` (`NODEINFO_BITFIELD_IS_FAVORITE_MASK`).
- `position` **moved out** of `NodeInfoLite` → `nodeDB->copyNodePosition(num, PositionLite&)`.
- `config.bluetooth.hide_pin` **removed upstream** → moved into InkHUD2's own settings; `inkhud2.dat` v2→**v3** (`[version][rotation][hidePIN]`, v2 still readable).
Files: `InkHUD2/Events.cpp`, `Modules/BootModule.cpp`, `Modules/MenuModule.cpp`, `Core/Settings.{h,cpp}`, `Setup.cpp`. Detail → `docs/PORT_THINKNODE_M1.md`.
⚠️ **Lesson:** after any upstream base bump, build at least one InkHUD2 env before assuming the tree is healthy.

## Status legend
- ✅ done & verified · 🟡 WIP/behind-flag · 🔴 bug/blocker · 🧪 untested · ⚙️ patch/config · 📝 design decision

---

## Project map
- **Main dev branch / path:** `/Users/mysinpyu/Claude/firmware_inkhud2_dev`
- **Base upstream:** Meshtastic **2.8.0** (`version.properties`; bumped 2026-06-16 — this is the bump that caused the protobuf drift above)
- **Sibling repos:**
  - `_backup_node` — old InkHUD with backup system (synced with `kuroanji/firmware` `auto_backup` branch)
  - `_release_inkhud2` — release copy (synced with `kuroanji/firmware` `InkHUD2` branch)
  - `_kuroanji_auto_backup` — clone of `kuroanji/firmware` `auto_backup` branch
- **Key source dirs:**
  - `src/graphics/niche/InkHUD2/` — main dir
    - `Core/` — Buffer, Font, Layout, RenderContext
    - `Modules/` — BatteryModule, MessageModule, NodeListModule, MenuModule, MapModule, …
    - `Pipe/` — `Pipe.cpp` (module management), `Events.cpp` (Meshtastic link)
    - `UI/` — shared UI components (see below)
  - `variants/nrf52840/`, `variants/esp32s3/` — per-target config; `variants/.../nicheGraphics.h` = init + button setup

## Build & flash (quick-ref)
```bash
# nRF52840 → produces .uf2
pio run -e t-echo-inkhud2
pio run -e t-echo-plus-inkhud2
pio run -e heltec-mesh-pocket-qi2-10000-inkhud2   # Qi2 with 10000mAh battery
pio run -e heltec-mesh-pocket-qi2-5000-inkhud2    # Qi2 with 5000mAh battery
pio run -e tlora-t3s3-epaper-inkhud2   # LilyGo T3-S3 e-paper 2.13" (ESP32-S3)
pio run -e tracker-t1000-e   # SenseCAP T1000-E (no screen; backup + i2c-rescue build, see T1000-E section)
pio run -e thinknode_m1-inkhud2   # Elecrow ThinkNode M1 (see docs/PORT_THINKNODE_M1.md)

# ESP32-S3 → produces .factory.bin
pio run -e heltec-vision-master-e290-inkhud2
pio run -e heltec-vision-master-e213-inkhud2
pio run -e heltec-wireless-paper-inkhud2
```
- **Artifact (nRF):** `.pio/build/<env>/firmware-<env>-*.uf2`
- **Flash gotchas:**
  - T-Echo Plus bootloader volume name **varies** (TECHOBOOT, TECHBOOT, …) — always check `ls /Volumes/`.
  - Mesh Pocket bootloader: volume `HT-n5262`, enter via **double-click Reset**.
  - ⚙️ Copy to `/Volumes/TECH*/` **without asking permission** — flash immediately.
  - `fcopyfile failed: Input/output error` on UF2 copy is **NORMAL** — it means flashing started.

## Targets / devices
| Env | Device | Platform | Screen |
|---|---|---|---|
| `t-echo-inkhud2` | LilyGo T-Echo | nRF52840 | 1.54" 200×200 |
| `t-echo-plus-inkhud2` | LilyGo T-Echo Plus | nRF52840 | 1.54" 200×200 |
| `heltec-mesh-pocket-qi2-10000-inkhud2` | Heltec Mesh Pocket Qi2 (10000mAh) | nRF52840 | 2.13" 122×250 |
| `heltec-mesh-pocket-qi2-5000-inkhud2` | Heltec Mesh Pocket Qi2 (5000mAh) | nRF52840 | 2.13" 122×250 |
| `heltec-vision-master-e290-inkhud2` | Heltec VM E290 | ESP32-S3 | 2.9" 128×296 |
| `heltec-vision-master-e213-inkhud2` | Heltec VM E213 | ESP32-S3 | 2.13" 250×122 |
| `heltec-wireless-paper-inkhud2` | Heltec Wireless Paper | ESP32-S3 | 2.13" 250×122 |
| `tlora-t3s3-epaper-inkhud2` | LilyGo T3-S3 e-paper | ESP32-S3 | 2.13" 250×122 |
| `tracker-t1000-e` | Seeed SenseCAP T1000-E | nRF52840 | **none** (tracker; built from this tree for backup + i2c fix, NOT an InkHUD UI target) |
| `thinknode_m1-inkhud2` | Elecrow ThinkNode M1 | nRF52840 | 1.54" 200×200 (same panel as T-Echo) — ✅ verified on HW; rotation **0**; front light **auto-off after 30 s idle** (`backlightIdleOffMs`, core feature) → aux button = scroll. Bootloader vol `ELECROWBOOT`. → `docs/PORT_THINKNODE_M1.md` |

## Hardware reference (verified)
### T-Echo Plus
- MCU **nRF52840** · LoRa **SX1262** · GPS **L76KB** (Quectel) · RTC **PCF8563** · Haptic **DRV2605** · optional **BME280** (temp/pressure).
- IMU **BHI260AP** (Bosch 6-axis) — **NOT supported by Meshtastic** (see roadmap).
- Screen 200×200 e-ink.
- Battery: ADC on **PIN_A0**, divider 100K+100K, charger **TP4054**. ⚠️ Charging **cannot** be disabled in software (no enable pin). On USB, Vbat reads = charge voltage (~4.2V).

### Heltec Mesh Pocket Qi2
- Screen 2.13" e-ink (122×250), driver **LCMEN2R13ECC1** (WISEVAST, **SSD1680** controller). Rotation 3 (landscape 250×122).
- Single button (`PIN_BUTTON1`), no aux button, no backlight. `hw_model` = **94** (HELTEC_MESH_POCKET).
- 📝 **Ghosting (hardware limitation, not a firmware bug):** display has bad OTP waveform → ghosting on FAST refresh. Custom LUT does **not** work (screen freezes — format incompatible). Stock Heltec firmware has the same issue (worse). **Fix:** `config.idleFullRefreshMs = 60000` (FULL refresh after 60s idle) + temperature sensor enabled (0x18 → 0x80) for temp adaptation.

## Architecture & components
- **Architecture doc:** `src/graphics/niche/InkHUD2/ARCHITECTURE.md` — composition over inheritance (RenderContext is **passed**, not inherited); shared UI (no code dup); priority-based overlays for SystemModule; slot-based layout (1/2/4 slots).
- **Shared UI** (`src/graphics/niche/InkHUD2/UI/`, used by MenuModule, MapModule, NodeListModule, MessageModule):
  - **StatusBar** — header: icon + title (ENVELOPE, USERS, GEAR, INFO, MAP)
  - **Footer** — hint bar at bottom
  - **ContentArea** — computes area between header and footer
  - **MenuList** — reusable menu renderer with navigation
  - **MenuItem** — types: ACTION, TOGGLE, VALUE, SUBMENU, BACK, LABEL
- **MapModule** ✅ — GPS nodes on 2D map; transform lat/lng → bearing/distance → north/east → pixels; node clustering; scale bar; bullseye for own position; settings submenu (show all / favorites only).

## Subsystems / feature notes
| Subsystem | Key fact (1 line) | Source | Status |
|---|---|---|---|
| CJK font | `UnifiedFont18px.h` — ~2900 glyphs 18×18px, ASCII+Cyrillic+JP+CN+KR; glyph = `{codepoint, bitmapOffset, xAdvance}` | generator `/Users/mysinpyu/Claude/font-generator/` | ✅ |
| Font-gen nuances | global `yOffset=-18` (not per-glyph); two paths Latin(cp<0x3000)/CJK(cp≥0x3000); **U+2026 (…) needs CJK path** (`is_cjk = … or cp in {0x2026,0x2025}`) else renders centered not on baseline; JP punctuation bottom/top/center | `font-generator/README.md` | ✅ |
| E-ink refresh | differential: "old memory" (reg 0x26) vs "new memory" (0x24); `finalizeUpdate()` copies current→old after FAST; **never change buffer while `busy()==true`** (→ ghosting); auto deep-sleep after each update | — | ✅ |
| Idle maintenance | `idleFullRefreshMs`: 0=off (default), >0 = one-shot FULL after N ms idle (resets on input); for bad-differential displays (LCMEN2R13ECC1); no effect on T-Echo Plus | — | ✅ |
| E-ink polling timeout | `EInk.h` `pollingTimeout=10000` (10s default); **3-color displays need 15–20s**; slow drivers must set it in `detachFromUpdate()` (e.g. `pollingTimeout=25000; return beginPolling(200,12000);`); all current B/W drivers ≤3.5s | `EInk.h` | ✅ |
| Menu shutdown | states `SHUTDOWN` ("Shutting down…") → `SHUTDOWN_FINAL` (logo + node name); global `::shutdownAtMsec` (**not** `InkHUD2::shutdownAtMsec`); save data **before** animation (guards against accidental reset); backup via `nodeDB->backupPreferences(FLASH)` → `/backups/backup.proto` | — | ✅ |
| Power-loss protection | backup on shutdown (`Power::shutdown`→`backupPreferences`, core, all targets) + admin backup/restore; 3-level rotation (auto/auto_prev/user .proto). 🆕 **Auto boot-restore NOW wired in core** `NodeDB::loadFromDisk`: a corrupt config/moduleConfig/channels load sets a `corruptSettingsMask`, and after the load a guarded `restorePreferences(FLASH, mask)` self-heals from the newest good backup (no-op on first boot / no backup). Before this it was **only manual (InkHUD menu) / admin** — screenless targets (T1000-E) had no self-heal. Target-independent (applies to any target, but we do NOT upstream — fork-only) | design doc in global memory `backup-restore.md` | ✅ |
| Battery icon style | bump (+ terminal) left 2px; body right, outline; hatch fill right→left; dashed divider (every other pixel) | — | ✅ |

## Gotchas & patch log ⚙️
- `LED_BUILTIN=-1` — **must** add to `build_flags` for nRF52 InkHUD2 builds (`nrf52_base` doesn't define it).
- `Adafruit SSD1306`, `Adafruit SH110X` — add to `lib_ignore` for ESP32 builds.
- 📝 **Dependency optimization** — motion-sensor libs **removed** (MPU6050, LIS3DH, LSM6DS, ICM20948, BMM150), saves ~17KB flash. Rationale: T-Echo Plus has BHI260AP but Meshtastic doesn't support it; it would only give wake-on-motion (no magnetometer = no compass), which is pointless for an always-visible e-ink screen. Conditional compile via `__has_include()`:
  - `AccelerometerThread.h` — MPU6050, LIS3DH, LSM6DS3, ICM20948, ICM42607P, BMM150, BMX160
  - `MagnetometerThread.h` — MMC5983MA

## Solved problems (post-mortem log)
| Problem | Cause | Fix | Result |
|---|---|---|---|
| CJK/Cyrillic not rendering | placeholder glyph path | use CJKFont instead | ✅ fixed |
| Scaled text broken | `uint8_t` for yOffset | `int8_t` | ✅ fixed |
| `hatch()` ignored clip | missing coord transform | add clipX/clipY conversion | ✅ fixed |
| E-ink ghosting | race: buffer changed during `finalizeUpdate()` | `driver->busy()` check before render in `runOnce()` | ✅ fixed |
| Too-frequent FULL refresh | automatic counter | removed; FULL only on explicit request | ✅ fixed |
| LittleFS `FILE_O_WRITE` doesn't truncate | open-for-write keeps old tail | `FSCom.remove()` before write | ✅ fixed |

## Tooling
- **Serial monitor without TTY** (Claude Code has no TTY → `pio device monitor` / `screen` don't work):
  ```bash
  stty -f /dev/cu.usbmodem1301 115200 && cat /dev/cu.usbmodem1301 | head -100
  ```
  Run with `run_in_background: true`, then read the output file. Port: `/dev/cu.usbmodem1301` (check `ls /dev/cu.usb*`).

---

## 📟 Non-InkHUD target: SenseCAP T1000-E (`tracker-t1000-e`) — backup + i2c rescue
**→ now also in-repo: `docs/PORT_T1000E.md`** (keep the two in sync; the doc is the public copy)
Built from THIS tree (no screen → not an InkHUD UI target). Purpose: ship our auto-backup + core fixes on the Seeed T1000-E tracker. Verified end-to-end on 2026-07-04.
- **Build:** `pio run -e tracker-t1000-e` → `.pio/build/tracker-t1000-e/firmware-…-.uf2`. hw_model 71, nRF52840, SoftDevice **S140 7.3.0**, LoRa **LR1110**, GPS **AG3335** (UART), accel **QMA6100P** (i2c, `HAS_QMA6100P`).
- **Flash (DFU):** hold button → connect USB cable **twice quickly** → green LED solid = DFU (`/Volumes/T1000-E`); drag-drop `.uf2` (`fcopyfile … Input/output error` = NORMAL = flash started). Hard power-cycle: unplug USB → hold button → plug while holding ~3s.
- **Erase-first (official Seeed step, do BEFORE firmware for a clean slate):** flash `bin/Meshtastic_nRF52_factory_erase_v3_S140_7.3.0.uf2` (must match SoftDevice **S140 7.3.0**) — wipes FS/config/keys/BLE-bonds, then device returns to DFU. NOTE: erase is flash-only, does **not** clear a hardware-wedged i2c sensor.
- ⚙️ **BLE:** no screen → fixed pin **123456**. ⚠️ **USB↔BLE contention:** a real serial PhoneAPI client (`meshtastic --info`) holds the single phone slot → app hangs on "**Communicating**" after PIN. Fix: **unplug USB**, connect over BT. (Raw serial-log `cat`/pyserial does NOT block BLE — only a protocol client does.)
- 🔧 **Serial log w/o TTY:** `timeout` is absent on macOS; use pyserial from the meshtastic pipx venv (`~/.local/pipx/venvs/meshtastic/bin/python`, DTR high) or `cat`+`kill`, never `timeout cat`.

### 🔴→✅ Fix: T1000-E boot-loop at i2c scan (meshtastic/firmware#4615)
- **Symptom:** hangs during `Scan for i2c devices` → no serial API, no BLE (setup() never finishes) → watchdog → bootloader. **Root:** a wedged i2c sensor holds SDA low; Adafruit nRF52 `TwoWire::endTransmission()` spins in an **unbounded `while(!EVENTS_STOPPED)`** loop → infinite hang on the first probe. Survives warm reset & full erase (sensor is battery-powered; erase is flash-only).
- **Fix — `src/main.cpp`, guarded `#if defined(TRACKER_T1000_E)`, runs before `Wire.begin()`:**
  1. **Sensor power-cycle** — `PIN_3V3_EN`(P1.6)+`T1000X_SENSOR_EN_PIN`(P0.4) LOW 120ms → HIGH, to hard-reset a healthy-but-stuck sensor. GENTLE cut only — do **NOT** ground SDA/SCL during the cut (that half-revives a marginal sensor into a "reads-OK-at-idle-but-still-hangs-TWIM" state, defeating the skip).
  2. `i2cBusRecover()` — if SDA still low, bit-bang ≤9 SCL pulses + STOP; returns whether SDA released.
  3. **Skip-scan safety net** — `i2cScanner->scanPort(WIRE)` wrapped in `if (i2cBusUsable)`; if still wedged, skip so the device still boots (no i2c sensors that session).
- **Outcome:** healthy unit → power-cycle recovers sensor → scan runs → accelerometer works. Wedged unit → skip → boots fully (LoRa/GPS/BLE). Log tells: `I2C bus stuck … STILL STUCK` → `skipping scan` → `Advertise`.
- ⚠️ **This physical unit's QMA6100P = DEAD HARDWARE:** SDA stayed stuck through power-cycle, aggressive depower (SDA/SCL grounded + 300ms), a bounded bit-bang health-probe, AND full factory erase. Unfixable in firmware (matches #4615 where erase also failed) → accelerometer/motion-wake gone on this board only; all else works. **Rejected approaches (both let boot reach the scan then hang the TWIM):** grounded-line depower; bit-bang health-probe (passed idle+probe yet TWIM still hung). Reverted to gentle-depower + SDA-stuck-skip = reliable.
- **MeshCore on T1000-E would NOT hang:** its `variants/t1000-e` never does a blocking i2c transaction at boot — `Wire.begin()` is peripheral-config only, sensors are ADC (temp/light) + UART (GPS) + SPI (LoRa), QMA6100P unused, no i2c scan. Meshtastic hangs *only* because of its boot-time i2c address scan.

## 🔴 TODO / roadmap
1. **📝 BHI260AP PDR integration** (future) — Pedestrian Dead Reckoning + GPS fusion; open SDK (BSD-3-Clause), nRF52 port exists; enables GPS duty-cycling (power saving) + better position accuracy. Est. ~1–2 weeks. → `docs/FUTURE_BHI260AP_PDR.md`
2. **📝 Power-loss backup/restore** — 🆕 auto boot-restore now wired in core `loadFromDisk` (was manual/admin only). Lives in `NodeDB.cpp`, i.e. core, so watch it on every base bump (same class of risk as the retired `hide_pin` protobuf patch). ⚠️ **No upstream PR** — fork-only, see Project rules. → `backup-restore.md`, T1000-E section above

---

## Project rules
- **All repo docs in English** (every `.md`). **Code comments in English.**
- 🔴 **Git: we work ONLY with the fork `kuroanji/firmware`.** Remotes (fixed 2026-07-18):
  `origin` = https://github.com/kuroanji/firmware (push here), `upstream` =
  meshtastic/firmware, **read-only** — its push URL is set to `no_push`, so a stray
  `git push upstream` fails locally instead of hitting the network. Our branch
  `update-inkhud2` tracks `origin/InkHUD2`; since the names differ, `push.default` is
  set to `upstream` (repo-local) — without it a bare `git push` aborts. So plain
  `git push` is now correct and safe. `upstream` fetches only `develop`/`master` + the pinned
  base tag. 🔴 **No upstream PRs, ever** — we never push or open a PR against
  meshtastic. Some of our work is target-independent (backup/restore, the T1000-E i2c
  fix) and *would* apply upstream; that is a description of the code, not a plan.
  Consequence: core-file changes stay ours to re-apply on every base bump — keep them
  listed under Gotchas & patch log.

## File map
```
firmware_inkhud2_dev/
├── INDEX.md                                   ← navigator (this file)
├── src/graphics/niche/InkHUD2/
│   ├── ARCHITECTURE.md                        core design principles
│   ├── Core/  Modules/  Pipe/  UI/            engine / features / glue / shared UI
├── variants/{nrf52840,esp32s3}/              per-target config + nicheGraphics.h
└── docs/FUTURE_BHI260AP_PDR.md               roadmap deep-dive
```
*(Power-loss backup design lives in global memory `backup-restore.md`, not in-repo.)*
