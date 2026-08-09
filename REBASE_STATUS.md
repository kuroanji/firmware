# REBASE STATUS — InkHUD2 → upstream 6eac181

**Goal:** move our InkHUD2 fork onto fresh upstream base `6eac181` (which has working
T3-S3 radio-reconfigure), instead of patching our stale base. Root cause of the T3-S3
radio-reconfigure crash = our base drifted far behind upstream (radio/NodeDB refactors).

## Branches & backup
- Working branch: **`rebase/inkhud2-on-6eac181`** (created from `6eac181`).
- Old fork state: **`update-inkhud2`** @ `dceb72e1d` (today's T3-S3 fixes committed there).
- Backup pointer: **`backup/pre-rebase-2026-08-09`**.
- Tar backup: `/Users/mysinpyu/Claude/firmware_inkhud2_dev_BACKUP_2026-08-09.tar.gz` (81M, verified).

## Method
git-rebase impossible (1068-file drift). Instead: fresh branch from 6eac181, then
cherry-port our layer file-by-file, fixing base-API drift as it surfaces.
`git checkout update-inkhud2 -- <path>` pulls our version of a file onto the new base.

## DONE ✅
- InkHUD2 engine: `src/graphics/niche/InkHUD2/` (65 files) — ported, compiles.
- CJK fonts: `src/graphics/niche/Fonts/CJK/` (7 files) — ported.
- Driver `HeltecVME290.{cpp,h}` — ported.
- T3-S3 configs: our `nicheGraphics.h` (InkHUD2 branch), `platformio.ini` (inkhud2 env),
  `InkHUD/PlatformioConfig.ini` (inkhud base env) — ported.
- NOTE: 6eac181 base ALREADY has today's fixes (extern SPI_HSPI, partition-table-t3s3.csv,
  &SPI_HSPI reuse) — they were restored FROM it, so no need to re-add on this branch.

## DONE ✅ (cont.) — T3-S3 WORKS on new base @ commit 44627d039
- backup/restore core patches ported into 6eac181 NodeDB:
  `backupNodeDatabase()` + `backupUserPreferences()` + `userBackupFileName` const (public decls).
- T3-S3 `tlora-t3s3-epaper-inkhud2`: **compiles, flashes, boots, BLE/screen/menu OK**.
- 🎯 **radio-reconfigure crash GONE** — user confirmed no reboot on BLE connect. This was
  the whole reason for the rebase (old base drifted behind upstream radio fixes).
- ⚠️ backup/restore NOT fully done: only the two methods InkHUD2 needs. Auto boot-restore
  (corruptSettingsMask in loadFromDisk, T1000-E self-heal) + faithful position projection
  in backupNodeDatabase still TODO.

## DONE ✅ (cont.) — ALL 8 InkHUD2 targets build @ commit f9bc10956
- esp32s3: heltec e290, e213, wireless-paper — SUCCESS.
- nrf52840: t-echo, t-echo-plus, mesh-pocket, thinknode-m1 — SUCCESS.
- Motion fix: ported our `MMC5983MA __has_include` guard in `MagnetometerThread.h`;
  kept `AccelerometerThread.h` UPSTREAM (it gained `providesHeading` — do NOT replace with ours).

## DONE ✅ (cont.) — T1000-E i2c-rescue @ commit 7fb543767
- i2cBusRecover() + power-cycle + skip-scan ported into drifted main.cpp (guarded TRACKER_T1000_E).
- tracker-t1000-e builds; t-echo regression clean.
- **ALL 9 firmware envs now build on 6eac181** (8 InkHUD2 + T1000-E).
- hide_pin: already handled — it lives in InkHUD2 engine/Settings, ported with the engine.

## DONE ✅ — auto boot-restore @ commit 9df45b57b
- corruptSettingsMask in loadFromDisk (config/moduleConfig/channels DECODE_FAILED) + restore block.
- Adapted to 6eac181 single backupFileName (our 3-tier rotation not carried; uses backupFileName + userBackupFileName).
- Lifts upstream configDecodeFailed freeze on successful restore. Compiles (t1000+t-echo clean).
- ⚠️ NOT HW-tested functionally (corruption hard to repro) — same caveat as always.

## ✅✅ FIRMWARE REBASE COMPLETE — all functional layers on 6eac181
Commits on `rebase/inkhud2-on-6eac181`: 44627d0 → f9bc109 → 7fb5437 → 9df45b5.
All 9 envs build; T3-S3 HW-verified (radio-reconfigure crash gone).

## REMAINING (non-firmware, optional)
- **tooling**: mcp-server/, docs/, .claude/, INDEX.md — doesn't affect firmware builds. Port last.

## TODO (ordered)
1. **backup/restore** → merge into 6eac181 NodeDB: `backupNodeDatabase()`, `corruptSettingsMask`,
   auto boot-restore in `loadFromDisk`. (task #3)
2. Get T3-S3 to fully compile (iterate build → port next missing symbol). (task #2)
3. **Flash+verify T3-S3** — boot/BLE/screen/menu + **radio-reconfigure no longer crashes**
   (the whole point). (task #5)
4. **T1000-E i2c-rescue** → merge into 6eac181 `main.cpp`. (task #4)
5. **hide_pin** patch (Settings).
6. **Remaining 7 targets** — nicheGraphics InkHUD2 branches + inkhud2 envs
   (t-echo, t-echo-plus, mesh-pocket, heltec e290/e213/wireless-paper, thinknode-m1) + T1000-E env.
7. **Tooling** (non-firmware, port last): `mcp-server/`, `docs/`, `.claude/`, INDEX.md.
8. Build + HW-verify each target.

## Serial capture (no TTY here)
```
~/.local/pipx/venvs/meshtastic/bin/python -c "import serial,time; s=serial.Serial('/dev/cu.usbmodem101',115200,timeout=1); t=time.time()
while time.time()-t<30:
 l=s.readline().decode('utf-8','replace').rstrip()
 if l: print(l)
s.close()"
```
Decode backtrace: `~/.platformio/packages/toolchain-xtensa-esp-elf/bin/xtensa-esp32s3-elf-addr2line -pfiaC -e .pio/build/<env>/*.elf <addrs>`
Flash needs manual download mode (hold BOOT, tap RST) when device is bootlooping.
