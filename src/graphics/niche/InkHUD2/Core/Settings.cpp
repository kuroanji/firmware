/*
* This is a personal academic project. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
*/
#include "Settings.h"
#include "FSCommon.h"
#include <Arduino.h>

namespace InkHUD2 {

void Settings::load() {
    auto file = FSCom.open(SETTINGS_FILE, FILE_O_READ);
    if (!file) {
        return;
    }

    // Binary format: [version:1][rotation:1] (+[hidePIN:1] since v3)
    uint8_t version = 0;
    if (file.read(&version, 1) != 1) {
        file.close();
        return;
    }

    if (version == 2 || version == 3) {
        // Version 2+: rotation is offset from device default
        uint8_t rot = 0;
        if (file.read(&rot, 1) == 1) {
            rotation = rot % 4;
            rotationWasSet = true;
        }
        if (version >= 3) {
            // Version 3 added hidePIN (moved here from config.bluetooth.hide_pin,
            // which no longer exists in upstream protobufs)
            uint8_t hp = 0;
            if (file.read(&hp, 1) == 1) {
                hidePIN = (hp != 0);
            }
        }
    }
    // Version 1 was broken, ignore and use defaults

    file.close();
}

void Settings::save() {
    // Remove old file first - FILE_O_WRITE doesn't truncate properly on LittleFS
    FSCom.remove(SETTINGS_FILE);

    auto file = FSCom.open(SETTINGS_FILE, FILE_O_WRITE);
    if (!file) {
        return;
    }

    // Binary format: [version:1][rotation:1][hidePIN:1]
    // Version 3: rotation is offset from device default; hidePIN moved here
    // from config.bluetooth.hide_pin (field removed upstream)
    uint8_t version = 3;
    file.write(&version, 1);
    file.write(&rotation, 1);
    uint8_t hp = hidePIN ? 1 : 0;
    file.write(&hp, 1);
    file.flush();
    file.close();
}

} // namespace InkHUD2
