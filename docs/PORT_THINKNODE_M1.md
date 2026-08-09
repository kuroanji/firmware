# Port: Elecrow ThinkNode M1 → InkHUD2 (`thinknode_m1-inkhud2`)

Date: 2026-07-18. Status: ✅ built & verified on hardware.

## Why it was easy
M1's panel is **GDEY0154D67, 1.54" 200×200 — the same as T-Echo**, on SPI1. InkHUD2's
layout needed **zero** changes. Upstream already shipped a `nicheGraphics.h` (old InkHUD)
for this board, so only an InkHUD2 branch + env were required.

## Files touched
- `variants/nrf52840/ELECROW-ThinkNode-M1/platformio.ini` — new `[env:thinknode_m1-inkhud2]`
- `variants/nrf52840/ELECROW-ThinkNode-M1/nicheGraphics.h` — wrapped in
  `#ifdef USE_INKHUD2 / #else / #endif`; upstream InkHUD path left untouched in `#else`

## Hardware findings (verified on device)

| Item | Value | Notes |
|---|---|---|
| Panel | `GDEY0154D67` 200×200, SPI1 | same as T-Echo |
| **Rotation** | **0** | ✅ verified. Upstream's own M1 path uses `setRotation(4)` ≡ 0 (`EInkDisplay2.cpp`). On-device: default 3 was wrong, +90 menu offset fixed it → `(1+3)%4 = 0`. |
| Main button | `PIN_BUTTON2` (P1.07) | **TOP button, circle icon.** Elecrow manual calls it "Page Turn". |
| Aux button | `PIN_BUTTON1` (P1.10) | **BOTTOM button, triangle icon.** Manual: "Function". |
| **Backlight** | ❌ **not software-controllable** | see below |
| Buzzer | `PIN_BUZZER` (P0.06) | keep `nRF52_PWM` dep |

### Front light: two controls, split between hardware and firmware
The M1's light has **two independent controls** — confusing them cost us three wrong
conclusions before the hardware settled it:

| Control | What it does |
|---|---|
| `PIN_EINK_EN` (GPIO) | **on/off gate** — HIGH = lit, LOW = dark |
| rotary knob on the power switch | **brightness** — but it never reaches full off; at minimum the light still glows |

Elecrow's manual matches this split: the knob *"adjusts the backlight LED brightness
level"*, while a Function-button **double-click** toggles the light in stock firmware.
Upstream's *"hardware dimmable backlight … start enabled"* comment refers to the knob,
not to any PWM.

**Consequence:** you cannot leave the pin HIGH and let the knob be the switch — at the
knob's minimum the light still burns and drains the battery. And firmware can't detect
the knob position either: the M1 has exactly one ADC input (`NUM_ANALOG_INPUTS 1`) and
it's taken by the battery.

→ So the light is driven **automatically**, costing no button:

```cpp
config.backlightPin = PIN_EINK_EN;
config.hasBacklight = true;
config.backlightIdleOffMs = 30000;  // dark after 30 s idle; any input wakes it
```

`backlightIdleOffMs` is a **core InkHUD2 feature** added for this (see `InkHUD2::Config`),
not an M1 hack — it reuses the same `lastUserInputTime` counter as the anti-ghosting idle
refresh. Any device whose light lacks a hardware off position can use it. When it is
enabled, `Setup.cpp` deliberately does **not** bind the backlight to the aux button —
a self-managing light doesn't need one — so the aux button stays **scroll
(short = DOWN, long = UP)**.

### ⚠️ `getUserButtonPin()` is unusable here
M1's `variant.h` defines **no `BUTTON_PIN`** (only `PIN_BUTTON1/2` + `ALT_BUTTON_PIN`),
so `TwoButton::getUserButtonPin()` returns `0xFF`. Wire button pins **explicitly**.

### ⚠️ Pin overlap (harmless)
`PIN_BUTTON2` (P1.07) is *also* declared as `PIN_SPI1_MISO`. Harmless in practice —
the e-ink panel is write-only and MISO is never read. Button works normally.

## Flash budget — pick the right base
Original attempt used `nrf52840_base` (copying M1's upstream envs) → **94.5% flash**.
`nrf52840_base` = `nrf52_base` + `environmental_base` + `environmental_extra`, i.e. a pile
of sensor drivers (BME280/BMP280/BMP085/DPS310/Bosch + Adafruit GFX + SSD1306). **M1 has
no onboard environmental sensors** (bare I2C bus only), so it was all dead weight.

Switched to `nrf52_base` (same as t-echo / mesh-pocket InkHUD2 envs):

| Base | Flash |
|---|---|
| `nrf52840_base` | 769 968 B — **94.5%** |
| `nrf52_base` | 643 100 B — **78.9%** |

**−124 KB.** When extending `nrf52_base` on an nRF52840, re-add `Adafruit_nRFCrypto`
manually (it comes from `nrf52840_base` otherwise).

`lib_ignore` must include `Adafruit SSD1306` / `Adafruit SH110X` if anything drags
`environmental_base` in — InkHUD2 uses GFXRoot, not Adafruit GFX.

## 🔴 Pre-existing breakage found & fixed (affected ALL InkHUD2 targets)
Building M1 exposed that **InkHUD2 did not compile at all** since the base was bumped to
Meshtastic 2.7.25 — the tree's protobufs had drifted. Not M1-specific; fixed in core:

| Was | Now | Files |
|---|---|---|
| `node->user.short_name` / `.long_name`, `has_user` | flattened: `node->short_name` / `node->long_name`; presence = `[0] != '\0'` | `Events.cpp`, `BootModule.cpp`, `MenuModule.cpp` |
| `node->is_favorite` | `node->bitfield & NODEINFO_BITFIELD_IS_FAVORITE_MASK` | `Events.cpp` |
| `node->position.*` | positions moved out of `NodeInfoLite` → `nodeDB->copyNodePosition(num, PositionLite&)` (canonical pattern, same as old InkHUD `MapApplet`) | `Events.cpp` |
| `config.bluetooth.hide_pin` | field removed upstream (only `fixed_pin` left) → moved into InkHUD2's own settings file | `Core/Settings.h/.cpp`, `Setup.cpp` |

`inkhud2.dat` format bumped **v2 → v3** (`[version][rotation][hidePIN]`); v2 files still
read (rotation preserved), so existing devices don't lose their setting. Persisting
hidePIN now uses `Settings::save()` instead of `nodeDB->saveToDisk()`.

## Gotchas for the next port
- **Rotation is an OFFSET**, not absolute: `initial = (menuOffset + defaultRotation) % 4`,
  offset persisted in `/prefs/inkhud2.dat`. If someone hand-corrected rotation on a build
  with a wrong default, they must reset the menu offset to 0 after the default is fixed —
  otherwise the correction is applied twice.
- Bootloader volume for M1 is **`ELECROWBOOT`** (not TECHOBOOT). `fcopyfile failed:
  Input/output error` on UF2 copy is normal — it means flashing started.
