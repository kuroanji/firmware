## Heltec VME290 Hybrid Driver (no ghosting)

TL;DR new **hybrid driver** for HeltecVME290 (ghosting eliminated, OTP LUT automatically adapts to temperature and is optimized for the specific panel).

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