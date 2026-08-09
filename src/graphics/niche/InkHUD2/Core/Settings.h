#pragma once

#include <cstdint>
#include "mesh/NodeDB.h"  // For extern config

namespace InkHUD2 {

// Global settings for InkHUD2
// Wrapper around Meshtastic config for type-safe access
// Some settings stored in Meshtastic config, others in separate file
class Settings {
public:
    static Settings& instance() {
        static Settings s;
        return s;
    }

    // Load/save InkHUD2-specific settings from/to file
    void load();
    void save();

    // PIN visibility on pairing screen (stored in InkHUD2 settings file).
    // NOTE: previously piggybacked on config.bluetooth.hide_pin, but that protobuf
    // field no longer exists upstream (only fixed_pin remains) — so we persist it
    // ourselves alongside rotation. See Settings::load/save (format version 3).
    bool getHidePIN() const { return hidePIN; }
    void setHidePIN(bool value) { hidePIN = value; }
    bool* hidePINPtr() { return &hidePIN; }

    // Screen rotation (0-3, stored in InkHUD2 settings file)
    uint8_t getRotation() const { return rotation; }
    void setRotation(uint8_t value) { rotation = value % 4; rotationWasSet = true; }
    uint8_t* rotationPtr() { return &rotation; }
    bool isRotationSet() const { return rotationWasSet; }

private:
    Settings() { load(); }
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;

    // InkHUD2-specific settings (not in Meshtastic config)
    uint8_t rotation = 0;
    bool rotationWasSet = false;  // True if loaded from file or explicitly set
    bool hidePIN = false;         // Hide BLE pairing PIN on screen

    static constexpr const char* SETTINGS_FILE = "/prefs/inkhud2.dat";
};

} // namespace InkHUD2
